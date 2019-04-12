// This is copyrighted software. More information is at the end of this file.
#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>
#include <QTextCodec>
#include <QTextLayout>
#include <QThread>
#include <QTimer>
#include <cstdarg>

#include "extcolors.h"
#include "happlication.h"
extern "C" {
#include "heheader.h"
}
#include "hframe.h"
#include "hmainwindow.h"
#include "hmarginwidget.h"
#include "hugodefs.h"
#include "hugohandlers.h"
#include "hugorfile.h"
#include "opcodeparser.h"
#include "settings.h"
#include "util.h"

#define INVOKE_BLOCK Qt::BlockingQueuedConnection

static const QLatin1String CONTROL_FNAME("HrCtlAPI");
static const QLatin1String CHECK_FNAME("HrCheck");

// Used to wait on the GUI thread for a condition.
static QMutex* waiterMutex = nullptr;

// Buffer for the script file. We don't immediately write text to the script file. We write to the
// buffer instead and flush it to the file when needed.
static QString* scriptBuffer = nullptr;

// Buffer for the scrollback. We flush it when needed.
static QByteArray* scrollbackBuffer = nullptr;

// Virtual control file for the Hugor handshake.
HugorFile& checkFile()
{
    static HugorFile f(nullptr);
    return f;
}

// Virtual control file for the Hugor extension opcode mechanism.
HugorFile& ctrlFile()
{
    static HugorFile f(nullptr);
    return f;
}

// Opcode parser.
OpcodeParser& opcodeParser()
{
    static OpcodeParser parser;
    return parser;
}

/* Helper routine. Converts a Hugo color to a Qt color.
 */
QColor hugoColorToQt(int color)
{
    color = static_cast<std::uint8_t>(color); // [-128..127] -> [0..255]
    QColor qtColor;

    switch (color) {
    case HUGO_BLACK:
        qtColor.setRgb(0x000000);
        break;
    case HUGO_BLUE:
        qtColor.setRgb(0x00007f);
        break;
    case HUGO_GREEN:
        qtColor.setRgb(0x007f00);
        break;
    case HUGO_CYAN:
        qtColor.setRgb(0x007f7f);
        break;
    case HUGO_RED:
        qtColor.setRgb(0x7f0000);
        break;
    case HUGO_MAGENTA:
        qtColor.setRgb(0x7f007f);
        break;
    case HUGO_BROWN:
        qtColor.setRgb(0x7f5f00);
        break;
    case HUGO_WHITE:
        qtColor.setRgb(0xcfcfcf);
        break;
    case HUGO_DARK_GRAY:
        qtColor.setRgb(0x3f3f3f);
        break;
    case HUGO_LIGHT_BLUE:
        qtColor.setRgb(0x0000ff);
        break;
    case HUGO_LIGHT_GREEN:
        qtColor.setRgb(0x00ff00);
        break;
    case HUGO_LIGHT_CYAN:
        qtColor.setRgb(0x00ffff);
        break;
    case HUGO_LIGHT_RED:
        qtColor.setRgb(0xff0000);
        break;
    case HUGO_LIGHT_MAGENTA:
        qtColor.setRgb(0xff00ff);
        break;
    case HUGO_YELLOW:
        qtColor.setRgb(0xffff00);
        break;
    case HUGO_BRIGHT_WHITE:
        qtColor.setRgb(0xffffff);
        break;
    case 16:
        qtColor = hApp->settings().main_text_color;
        break;
    case 17:
        qtColor = hApp->settings().main_bg_color;
        break;
    case 18:
        qtColor = hApp->settings().status_text_color;
        break;
    case 19:
        qtColor = hApp->settings().status_bg_color;
        break;
    case 20:
        qtColor = hApp->settings().main_text_color;
        break;

    default:
        if (color > 99 and color < 255) {
            qtColor = getExtendedColor(color);
        } else {
            qWarning() << Q_FUNC_INFO << "unknown color ID:" << color;
            qtColor.setRgb(0x000000);
        }
    }
    return qtColor;
}

static void flushScriptBuffer()
{
    const int wrapWidth = hApp->settings().script_wrap;

    // If wrapping is disabled, or the entire buffer is below our wrap limit, write out all text
    // as-is.
    if (wrapWidth <= 0 or scriptBuffer->length() <= wrapWidth) {
        fprintf(::script->get(), "%s", scriptBuffer->toLocal8Bit().constData());
        fflush(::script->get());
        scriptBuffer->clear();
        return;
    }

    QTextStream strm(scriptBuffer);
    QString textLine;
    while (not(textLine = strm.readLine()).isNull()) {
        // If the line fits and doesn't need wrapping, write it out as-is.
        if (textLine.length() < wrapWidth) {
            fprintf(::script->get(), "%s", textLine.trimmed().toLocal8Bit().constData());
            if (not strm.atEnd()) {
                fprintf(::script->get(), "\n");
            }
            continue;
        }

        QTextLayout layout(textLine);
        QTextOption txtOpts(Qt::AlignLeft);
        txtOpts.setWrapMode(QTextOption::WordWrap);
        layout.setTextOption(txtOpts);

        layout.beginLayout();
        QTextLine layoutLine;
        QString output;
        while ((layoutLine = layout.createLine()).isValid()) {
            layoutLine.setNumColumns(wrapWidth);
            output.append(textLine.midRef(layoutLine.textStart(), layoutLine.textLength()));
            output.append('\n');
        }
        layout.endLayout();
        fprintf(::script->get(), "%s", output.toLocal8Bit().constData());
    }

    fflush(::script->get());
    scriptBuffer->clear();
}

static void flushScrollbackBuffer()
{
    runInMainThread([] { hMainWin->appendToScrollback(*scrollbackBuffer); });
    scrollbackBuffer->clear();
}

void* hugo_blockalloc(long num)
{
    return new char[num];
}

void hugo_blockfree(void* block)
{
    delete[] static_cast<char*>(block);
}

/*
    FILENAME MANAGEMENT:

    Different operating systems have different ways of naming files.
    The following routines are simply required to know and be able to
    dissect/build the components of a particular filename,
    storing/restoring the components via the specified char arrays.

    For example, in MS-DOS:

    hugo_splitpath("C:\HUGO\FILES\HUGOLIB.H", ...)
        becomes:  C:, HUGO\FILES, HUGOLIB, H

    and

    hugo_makepath(..., "C:", "HUGO\FILES", "HUGOLIB", "H")
        becomes:  C:\HUGO\FILES\HUGOLIB.H

    The appropriate equivalent nomenclature should be used for the
    operating system in question.
*/

/* The following supplied functions will work for Unix-style pathnames: */

void hugo_splitpath(char* path, char* drive, char* dir, char* fname, char* ext)
{
    drive[0] = '\0';
    dir[0] = '\0';
    fname[0] = '\0';
    ext[0] = '\0';

    if (path[0] == '\0') {
        return;
    }

    QFileInfo inf(QString::fromLocal8Bit(path));
    qstrcpy(ext, inf.suffix().toLocal8Bit().constData());
    qstrcpy(fname, inf.completeBaseName().toLocal8Bit().constData());
    qstrcpy(dir, inf.absolutePath().toLocal8Bit().constData());
    // qDebug() << "splitpath:" << drive << dir << fname << ext;
}

void hugo_makepath(char* path, char* drive, char* dir, char* fname, char* ext)
{
    QByteArray result(drive);
    result += dir;
    if (not result.endsWith('/')) {
        result += '/';
    }
    result += fname;
    if (ext[0] != '\0') {
        result += '.';
        result += QByteArray(ext).toLower();
    }
    qstrcpy(path, result.constData());
}

/* hugo_getfilename

    Loads the name of the filename to save or restore (as specified by
    the argument <a>) into the line[] array.

    The reason this is in the system-specific file is because it may be
    preferable to replace it with, for example, a dialog-based file
    selector.
*/
void hugo_getfilename(char* a, char* b)
{
    runInMainThread([a, b] { HugoHandlers::getfilename(a, b); });
}

/* hugo_overwrite

    Checks to see if the given filename already exists, and prompts to
    replace it.  Returns true if file may be overwritten.

    Again, it may be preferable to replace this with something fancier.
*/
int hugo_overwrite(char* /*f*/)
{
    // We handle this in hugo_getfilename().
    return true;
}

static bool ctrlFileInWriteMode = false;

HUGO_FILE hugo_fopen(const char* path, const char* mode)
{
    if (QString(path).endsWith(CHECK_FNAME)) {
        if (mode[0] == 'r') {
            return &checkFile();
        }
        return nullptr;
    }
    if (QString(path).endsWith(CONTROL_FNAME)) {
        ctrlFileInWriteMode = mode[0] == 'w';
        return &ctrlFile();
    }
    auto handle = std::fopen(path, mode);
    if (handle == nullptr) {
        return nullptr;
    }
    return new HugorFile(handle);
}

int hugo_fclose(HUGO_FILE file)
{
    if (file == nullptr or file == &checkFile()) {
        return 0;
    }
    if (file == &ctrlFile()) {
        if (ctrlFileInWriteMode) {
            opcodeParser().parse();
        }
        return 0;
    }
    if (file == script) {
        flushScriptBuffer();
    }
    auto ret = file->close();
    delete file;
    return ret;
}

int hugo_fgetc(HUGO_FILE file)
{
    if (file == &checkFile()) {
        return 0x42;
    }
    if (file == &ctrlFile()) {
        if (opcodeParser().hasOutput()) {
            return opcodeParser().getNextOutputByte();
        }
        return EOF;
    }
    return std::fgetc(file->get());
}

int hugo_fseek(HUGO_FILE file, long offset, int whence)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return 0;
    }
    return std::fseek(file->get(), offset, whence);
}

long hugo_ftell(HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
    }
    return std::ftell(file->get());
}

size_t hugo_fread(void* ptr, size_t size, size_t nmemb, HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return 0;
    }
    return std::fread(ptr, size, nmemb, file->get());
}

char* hugo_fgets(char* s, int size, HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return nullptr;
    }
    return std::fgets(s, size, file->get());
}

int hugo_fputc(int c, HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        opcodeParser().pushByte(c);
        return c;
    }
    return std::fputc(c, file->get());
}

int hugo_fputs(const char* s, HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return 0;
    }
    return std::fputs(s, file->get());
}

int hugo_ferror(HUGO_FILE file)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return 0;
    }
    return std::ferror(file->get());
}

int hugo_fprintf(HUGO_FILE file, const char* format, ...)
{
    if (file == &ctrlFile()) {
        qDebug() << Q_FUNC_INFO;
        return 0;
    }
    va_list args;
    va_start(args, format);
    auto ret = std::vfprintf(file->get(), format, args);
    va_end(args);
    return ret;
}

/* hugo_closefiles

    Closes all open files.  Note:  If the operating system automatically
    closes any open streams upon exit from the program, this function may
    be left empty.
*/
void hugo_closefiles()
{
    delete game;
    hugo_fclose(script);
    delete io;
    delete record;
}

/* hugo_sendtoscrollback

   Stores a given line in the scrollback buffer (optional).
*/
void hugo_sendtoscrollback(char* a)
{
    scrollbackBuffer->append(a);
}

int hugo_writetoscript(const char* s)
{
    scriptBuffer->append(hApp->hugoCodec()->toUnicode(s));
    return 0;
}

/* hugo_getkey

    Returns the next keystroke waiting in the keyboard buffer.  It is
    expected that hugo_getkey() will return the following modified
    keystrokes:

    up-arrow        11 (CTRL-K)
    down-arrow      10 (CTRL-J)
    left-arrow       8 (CTRL-H)
    right-arrow     21 (CTRL-U)
*/
int hugo_getkey(void)
{
    if (::script != nullptr) {
        flushScriptBuffer();
    }
    flushScrollbackBuffer();

    QMutexLocker mLocker(waiterMutex);
    if (not hugo_iskeywaiting()) {
        hFrame->keypressAvailableWaitCond.wait(waiterMutex);
    }
    mLocker.unlock();

    int key = hFrame->getNextKey();
    if (key == 0) {
        // It's a mouse click.
        const QPoint& pos = hFrame->getNextClick();
        display_pointer_x = (pos.x() - physical_windowleft) / FIXEDCHARWIDTH + 1;
        display_pointer_y = (pos.y() - physical_windowtop) / FIXEDLINEHEIGHT + 1;
        return 1;
    }
    return key;
}

/* hugo_getline

    Gets a line of input from the keyboard, storing it in <buffer>.
*/
void hugo_getline(char* p)
{
    if (::script != nullptr) {
        hugo_writetoscript(p);
        flushScriptBuffer();
    }
    hugo_sendtoscrollback(p);
    flushScrollbackBuffer();

    QMutexLocker mLocker(waiterMutex);
    runInMainThread([p] { HugoHandlers::startGetline(p); });
    hFrame->inputLineWaitCond.wait(waiterMutex);
    hFrame->getInput(::buffer, MAXBUFFER);
    runInMainThread([] { HugoHandlers::endGetline(); });

    // Also copy the input to the script file (if there is one) and the scrollback.
    if (script != nullptr) {
        hugo_writetoscript(buffer);
        hugo_writetoscript("\n");
        flushScriptBuffer();
    }
    hugo_sendtoscrollback(buffer);
    char newline[] = "\n";
    hugo_sendtoscrollback(newline);
}

/* hugo_waitforkey

    Provided to be replaced by multitasking systems where cycling while
    waiting for a keystroke may not be such a hot idea.

    If kbhit() doesn't exist, has a different name, or has functional
    implications (i.e., on a multitasking system), this will have to
    be modified.
*/
int hugo_waitforkey(void)
{
    return hugo_getkey();
}

/* hugo_iskeywaiting

    Returns true if a keypress is waiting to be retrieved.
*/
int hugo_iskeywaiting(void)
{
    // qDebug(Q_FUNC_INFO);
    runInMainThread([] { hFrame->updateGameScreen(false); });
    return hFrame->hasKeyInQueue();
}

/* hugo_timewait

    Waits for 1/n seconds.  Returns false if waiting is unsupported.
*/
int hugo_timewait(int n)
{
    // qDebug() << Q_FUNC_INFO;
    if (hApp->gameRunning() and n > 0) {
        QThread::msleep(1000 / n);
        runInMainThread([] { hFrame->updateGameScreen(false); });
    }
    return true;
}

/* DISPLAY CONTROL:

   Briefly, the variables required to interface with the engine's output
   functions are:

   The currently defined window, in zero-based coordinates (either
   character coordinates or pixel coordinates, as appropriate):

    physical_windowleft, physical_windowtop,
    physical_windowright, physical_windowbottom,
    physical_windowwidth, physical_windowheight

   The currently selected font is described by the ..._FONT bitmasks in:

    currentfont

   The currently selected font's width and height:

    charwidth, lineheight

   The non-proportional/fixed-width font's width and height (i.e., equal
   to charwidth and lineheight when the current font is the fixed-width
   font):

    FIXEDCHARWIDTH, FIXEDLINEHEIGHT

   Must be set by hugo_settextpos(), hugo_clearfullscreen(), and
   hugo_clearwindow():

    currentpos, currentline
*/

/* Does whatever has to be done to initially set up the display.
 */
void hugo_init_screen(void)
{
    waiterMutex = new QMutex;
    scriptBuffer = new QString;
    scrollbackBuffer = new QByteArray;
}

/* Returns true if the current display is capable of graphics display;
 * returns 2 if graphics are being routed to an external window other
 * than the main display. */
int hugo_hasgraphics(void)
{
    if (hApp->settings().enable_graphics) {
        return 1;
    }
    return false;
}

void hugo_setgametitle(char* t)
{
    runInMainThread([t] { hMainWin->setWindowTitle(QLatin1String(t)); });
}

/* Does whatever has to be done to clean up the display pre-termination.
 */
void hugo_cleanup_screen(void)
{
    delete waiterMutex;
    delete scriptBuffer;
    delete scrollbackBuffer;
}

/* Clears everything on the screen, moving the cursor to the top-left
 * corner of the screen.
 */
void hugo_clearfullscreen(void)
{
    runInMainThread([] { HugoHandlers::clearfullscreen(); });
    currentpos = 0;
    currentline = 1;
    TB_Clear(0, 0, screenwidth, screenheight);
}

/* Clears the currently defined window, moving the cursor to the top-left
 * corner of the window.
 */
void hugo_clearwindow(void)
{
    runInMainThread([] { HugoHandlers::clearwindow(); });
    currentpos = 0;
    currentline = 1;
    TB_Clear(physical_windowleft, physical_windowtop, physical_windowright, physical_windowbottom);
}

/* This function does whatever is necessary to set the system up for
   a standard text display */

/*
                Pixel-based     Character-based
                -----------     ---------------

    FIXEDCHARWIDTH =    ...font width...         1
    FIXEDLINEHEIGHT =   ...font height...        1

    SCREENWIDTH =       ...# x pixels...        80
    SCREENHEIGHT =      ...# y pixels...        25

    As an example of how character-based and pixel-based
    systems might provide the same parameters, the following
    two sets of parameters are identical:

        FIXEDCHARWIDTH        8    1
        FIXEDLINEHEIGHT      16    1

        SCREENWIDTH     640   80  (640 /  8 = 80)
        SCREENHEIGHT        400   25  (400 / 16 = 25)

    Then set:

    charwidth = current font width, in pixels or 1
    lineheight = current font height, in pixels or 1

    Both charwidth and lineheight must change dynamically if the
    metrics for the currently selected font change
*/
void hugo_settextmode(void)
{
    runInMainThread([] { HugoHandlers::settextmode(); });
}

/* Once again, the arguments for the window are passed using character
   coordinates--a system setting a window using pixel coordinates will
   have to make the necessary conversions using FIXEDCHARWIDTH and
   FIXEDLINEHEIGHT.

   The text window, once set, represents the scrolling/bottom part of the
   screen.  It is also assumed that the text window will constrain the
   cursor location--see hugo_settextposition(), below.

   This function must also set physical_window... parameters using
   the transformations given.
*/
/* Create a text window from (column, row) character-coordinates
(left, top) to (right, bottom)
*/
void hugo_settextwindow(int left, int top, int right, int bottom)
{
    runInMainThread(
        [left, top, right, bottom] { HugoHandlers::settextwindow(left, top, right, bottom); });
}

/* The top-left corner of the current active window is (1, 1).

   (In other words, if the screen is being windowed so that the top row
   of the window is row 4 on the screen, the (1, 1) refers to the 4th
   row on the screen, and (1, 2) refers to the 5th.)

   All cursor-location is based on FIXEDCHARWIDTH and FIXEDLINEHEIGHT.

   This function must also properly set currentline and currentpos (where
   currentline is a the current character line, and currentpos may be
   either in pixels or characters, depending on the measure being used).

   Note that the Hugo function call uses x and y directly as text-
   screen coordinates; pixel-based systems will likely have to make a
   calculation to pixel coordinates.  In other words, pixel-based
   screen location will need to calculate pixel coordinates to
   simulate text-screen coordinates.
*/
void hugo_settextpos(int x, int y)
{
    // Must be set:
    currentline = y;
    currentpos = (x - 1) * ::charwidth; // Note:  zero-based

    // current_text_x/row are calculated assuming that the
    // character position (1, 1) is the pixel position (0, 0)
    current_text_x = physical_windowleft + currentpos;
    current_text_y = physical_windowtop + (y - 1) * lineheight;
}

/* PRINTFATALERROR may be #defined in heheader.h.
 */
void printFatalError(char* a)
{
    runInMainThread([a] { HugoHandlers::printFatalError(a); });
}

/* Essentially the same as printf() without formatting, since printf()
   generally doesn't take into account color setting, font changes,
   windowing, etc.

   The newline character '\n' must be explicitly included at the end of
   a line in order to produce a linefeed.  The new cursor position is set
   to the end of this printed text.  Upon hitting the right edge of the
   screen, the printing position wraps to the start of the next line.
*/
/* Output <a>, taking into account fore/background color,
   font, current window, etc.
*/
void hugo_print(char* a)
{
    runInMainThread([a] { HugoHandlers::print(a); });
}

/* Scroll the current text window up one line.
 */
void hugo_scrollwindowup()
{
    runInMainThread([] {
        hFrame->scrollUp(physical_windowleft, physical_windowtop, physical_windowright,
                         physical_windowbottom, lineheight);
    });
    TB_Scroll();
}

/* The <f> argument is a mask containing any or none of:
   BOLD_FONT, UNDERLINE_FONT, ITALIC_FONT, PROP_FONT.

   If charwidth and lineheight change with a font change, these must be
   reset here as well.
*/
void hugo_font(int f)
{
    runInMainThread([f] { HugoHandlers::font(f); });
}

void hugo_settextcolor(int c)
{
    runInMainThread([c] { HugoHandlers::settextcolor(c); });
}

void hugo_setbackcolor(int c)
{
    runInMainThread([c] { HugoHandlers::setbackcolor(c); });
}

/* CHARACTER AND TEXT MEASUREMENT

    For non-proportional printing, screen dimensions will be given
    in characters, not pixels.

    For proportional printing, screen dimensions need to be in
    pixels, and each width routine must take into account the
    current font and style.

    The hugo_strlen() function is used to give the length of
    the string not including any non-printing control characters.
*/
int hugo_charwidth(char a)
{
    if (a == FORCED_SPACE) {
        a = ' ';
    }
    if (static_cast<unsigned char>(a) < ' ') {
        return 0;
    }
    if (currentfont & PROP_FONT) {
        return hFrame->currentFontMetrics().width(hApp->hugoCodec()->toUnicode(&a, 1));
    }
    return FIXEDCHARWIDTH;
}

int hugo_textwidth(char* a)
{
    // With a fixed-width font, we know the width of the string is equal to its length times the
    // width of a character (all chars have the same width.)
    if (not(currentfont & PROP_FONT)) {
        return hugo_strlen(a) * FIXEDCHARWIDTH;
    }

    size_t slen = qstrlen(a);
    QString str;

    // Construct a string that contains only printable characters.
    for (size_t i = 0; i < slen; ++i) {
        if (a[i] == COLOR_CHANGE) {
            i += 2;
        } else if (a[i] == FONT_CHANGE) {
            ++i;
        } else {
            str += hApp->hugoCodec()->toUnicode(a + i, 1);
        }
    }
    return hFrame->currentFontMetrics().width(str);
}

int hugo_strlen(char* a)
{
    size_t len = 0;
    size_t slen = qstrlen(a);

    for (size_t i = 0; i < slen; ++i) {
        if (a[i] == COLOR_CHANGE) {
            i += 2;
        } else if (a[i] == FONT_CHANGE) {
            ++i;
        } else {
            ++len;
        }
    }
    return len;
}

int hugo_displaypicture(HUGO_FILE infile, long len)
{
    int result;
    runInMainThread([infile, len, &result] { HugoHandlers::displaypicture(infile, len, &result); });
    delete infile;
    return result;
}

int hugo_playmusic(HUGO_FILE infile, long len, char loop_flag)
{
    int result;
    runInMainThread([infile, len, loop_flag, &result] {
        HugoHandlers::playmusic(infile, len, loop_flag, &result);
    });
    delete infile;
    return result;
}

void hugo_musicvolume(int vol)
{
    runInMainThread([vol] { HugoHandlers::musicvolume(vol); });
}

void hugo_stopmusic(void)
{
    runInMainThread([] { HugoHandlers::stopmusic(); });
}

int hugo_playsample(HUGO_FILE infile, long len, char loop_flag)
{
    int result;
    runInMainThread([infile, len, loop_flag, &result] {
        HugoHandlers::playsample(infile, len, loop_flag, &result);
    });
    delete infile;
    return result;
}

void hugo_samplevolume(int vol)
{
    runInMainThread([vol] { HugoHandlers::samplevolume(vol); });
}

void hugo_stopsample(void)
{
    runInMainThread([] { HugoHandlers::stopsample(); });
}

#ifdef DISABLE_VIDEO

void muteVideo(bool)
{}

void updateVideoVolume()
{}

int hugo_hasvideo(void)
{
    return false;
}

void hugo_stopvideo(void)
{}

int hugo_playvideo(HUGO_FILE infile, long, char, char, int)
{
    delete infile;
    return true;
}

void initVideoEngine(int& /*argc*/, char* /*argv*/[])
{}

void closeVideoEngine()
{}

#else

int hugo_hasvideo(void)
{
    if (hApp->settings().enable_video and not hApp->settings().video_sys_error) {
        return true;
    }
    return false;
}

void hugo_stopvideo(void)
{
    runInMainThread([] { HugoHandlers::stopvideo(); });
}

int hugo_playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol)
{
    int result;
    runInMainThread([infile, len, loop, bg, vol, &result] {
        HugoHandlers::playvideo(infile, len, loop, bg, vol, &result);
    });
    delete infile;
    return result;
}

#endif

/* Copyright (C) 2011-2019 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Hugor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Hugor.  If not, see <http://www.gnu.org/licenses/>.
 */
