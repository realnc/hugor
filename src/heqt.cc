/* Copyright 2015 Nikos Chantziaras
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
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with the Hugo Engine (or a modified version of the Hugo Engine), containing
 * parts covered by the terms of the Hugo License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
#include <QDebug>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextLayout>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <QTextCodec>
#include <QMutexLocker>

#include "happlication.h"
#include "hmainwindow.h"
#include "hmarginwidget.h"
#include "hframe.h"
#include "settings.h"
#include "hugodefs.h"
#include "hugohandlers.h"

extern "C" {
#include "heheader.h"
}


#define INVOKE_BLOCK Qt::BlockingQueuedConnection

Q_DECLARE_METATYPE(HUGO_FILE)
Q_DECLARE_METATYPE(int*)
Q_DECLARE_METATYPE(char*)

// Exposes QThread::msleep(), which is protected.
class SleepFuncs: public QThread {
public:
    using QThread::msleep;
};

// Used to wait on the GUI thread for a condition.
static QMutex* waiterMutex = 0;

// Buffer for the script file. We don't immediately write text to the script
// file. We write to the buffer instead and flush it to the file when needed.
static QString* scriptBuffer = 0;

// Buffer for the scrollback. We flush it when needed.
static QByteArray* scrollbackBuffer = 0;


/* Helper routine. Converts a Hugo color to a Qt color.
 */
QColor
hugoColorToQt( int color )
{
    QColor qtColor;
    switch (color) {
      case HUGO_BLACK:         qtColor.setRgb(0x000000); break;
      case HUGO_BLUE:          qtColor.setRgb(0x00007f); break;
      case HUGO_GREEN:         qtColor.setRgb(0x007f00); break;
      case HUGO_CYAN:          qtColor.setRgb(0x007f7f); break;
      case HUGO_RED:           qtColor.setRgb(0x7f0000); break;
      case HUGO_MAGENTA:       qtColor.setRgb(0x7f007f); break;
      case HUGO_BROWN:         qtColor.setRgb(0x7f5f00); break;
      case HUGO_WHITE:         qtColor.setRgb(0xcfcfcf); break;
      case HUGO_DARK_GRAY:     qtColor.setRgb(0x3f3f3f); break;
      case HUGO_LIGHT_BLUE:    qtColor.setRgb(0x0000ff); break;
      case HUGO_LIGHT_GREEN:   qtColor.setRgb(0x00ff00); break;
      case HUGO_LIGHT_CYAN:    qtColor.setRgb(0x00ffff); break;
      case HUGO_LIGHT_RED:     qtColor.setRgb(0xff0000); break;
      case HUGO_LIGHT_MAGENTA: qtColor.setRgb(0xff00ff); break;
      case HUGO_YELLOW:        qtColor.setRgb(0xffff00); break;
      case HUGO_BRIGHT_WHITE:  qtColor.setRgb(0xffffff); break;
      case 16:                 qtColor = hApp->settings()->mainTextColor; break;
      case 17:                 qtColor = hApp->settings()->mainBgColor; break;
      case 18:                 qtColor = hApp->settings()->statusTextColor; break;
      case 19:                 qtColor = hApp->settings()->statusBgColor; break;
      case 20:                 qtColor = hApp->settings()->mainTextColor; break;
      default:                 qtColor.setRgb(0x000000);
    }
    return qtColor;
}


static void
flushScriptBuffer()
{
    const int wrapWidth = hApp->settings()->scriptWrap;

    // If wrapping is disabled, or the entire buffer is below our wrap limit,
    // write out all text as-is.
    if (wrapWidth <= 0 or scriptBuffer->length() <= wrapWidth) {
        fprintf(::script, "%s", scriptBuffer->toLocal8Bit().constData());
        fflush(::script);
        scriptBuffer->clear();
        return;
    }

    QTextStream strm(scriptBuffer);
    QString textLine;
    while (not (textLine = strm.readLine()).isNull()) {
        // If the line fits and doesn't need wrapping, write it out as-is.
        if (textLine.length() < wrapWidth) {
            fprintf(::script, "%s", textLine.trimmed().toLocal8Bit().constData());
            if (not strm.atEnd()) {
                fprintf(::script, "\n");
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
            output.append(textLine.mid(layoutLine.textStart(), layoutLine.textLength()));
            output.append('\n');
        }
        layout.endLayout();
        fprintf(::script, "%s", output.toLocal8Bit().constData());
    }

    fflush(::script);
    scriptBuffer->clear();
}


static void
flushScrollbackBuffer()
{
    QMetaObject::invokeMethod(hMainWin, "appendToScrollback", INVOKE_BLOCK,
                              Q_ARG(QByteArray, *scrollbackBuffer));
    scrollbackBuffer->clear();
}


void*
hugo_blockalloc( long num )
{
    return new char[num];
}


void
hugo_blockfree( void* block )
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

void
hugo_splitpath( char* path, char* drive, char* dir, char* fname, char* ext )
{
    drive[0] = '\0';
    dir[0] = '\0';
    fname[0] = '\0';
    ext[0] = '\0';

    if (path[0] == '\0')
        return;

    QFileInfo inf(QString::fromLocal8Bit(path));
    qstrcpy(ext, inf.suffix().toLocal8Bit().constData());
    qstrcpy(fname, inf.completeBaseName().toLocal8Bit().constData());
    qstrcpy(dir, inf.absolutePath().toLocal8Bit().constData());
    //qDebug() << "splitpath:" << drive << dir << fname << ext;
}


void
hugo_makepath( char* path, char* drive, char* dir, char* fname, char* ext )
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
void
hugo_getfilename( char* a, char* b )
{
    QMetaObject::invokeMethod(hHandlers, "getfilename", INVOKE_BLOCK, Q_ARG(char*, a), Q_ARG(char*, b));
}


/* hugo_overwrite

    Checks to see if the given filename already exists, and prompts to
    replace it.  Returns true if file may be overwritten.

    Again, it may be preferable to replace this with something fancier.
*/
int
hugo_overwrite( char* )
{
    // We handle this in hugo_getfilename().
    return true;
}


int
hugo_fclose( HUGO_FILE file )
{
    if (file == script) {
        flushScriptBuffer();
    }
    return fclose(file);
}


/* hugo_closefiles

    Closes all open files.  Note:  If the operating system automatically
    closes any open streams upon exit from the program, this function may
    be left empty.
*/
void
hugo_closefiles()
{
    fclose(game);
    if (script) {
        hugo_fclose(script);
    }
    if (io)
        fclose(io);
    if (record)
        fclose(record);
}


/* hugo_sendtoscrollback

   Stores a given line in the scrollback buffer (optional).
*/
void
hugo_sendtoscrollback( char* a )
{
    scrollbackBuffer->append(a);
}


int
hugo_writetoscript(const char* s)
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
int
hugo_getkey( void )
{
    if (::script != 0) {
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
void
hugo_getline( char* p )
{
    if (::script != NULL) {
        hugo_writetoscript(p);
        flushScriptBuffer();
    }
    hugo_sendtoscrollback(p);
    flushScrollbackBuffer();

    QMutexLocker mLocker(waiterMutex);
    QMetaObject::invokeMethod(hHandlers, "startGetline", INVOKE_BLOCK, Q_ARG(char*, p));
    hFrame->inputLineWaitCond.wait(waiterMutex);
    hFrame->getInput(::buffer, MAXBUFFER);
    QMetaObject::invokeMethod(hHandlers, "endGetline", INVOKE_BLOCK);

    // Also copy the input to the script file (if there is one) and the
    // scrollback.
    if (script != NULL) {
        hugo_writetoscript(buffer);
        hugo_writetoscript("\n");
        flushScriptBuffer();
    }
    hugo_sendtoscrollback(buffer);
    hugo_sendtoscrollback(const_cast<char*>("\n"));
}


/* hugo_waitforkey

    Provided to be replaced by multitasking systems where cycling while
    waiting for a keystroke may not be such a hot idea.

    If kbhit() doesn't exist, has a different name, or has functional
    implications (i.e., on a multitasking system), this will have to
    be modified.
*/
int
hugo_waitforkey( void )
{
    return hugo_getkey();
}


/* hugo_iskeywaiting

    Returns true if a keypress is waiting to be retrieved.
*/
int
hugo_iskeywaiting( void )
{
    //qDebug(Q_FUNC_INFO);
    QMetaObject::invokeMethod(hFrame, "updateGameScreen", INVOKE_BLOCK, Q_ARG(bool, false));
    return hFrame->hasKeyInQueue();
}


/* hugo_timewait

    Waits for 1/n seconds.  Returns false if waiting is unsupported.
*/
int
hugo_timewait( int n )
{
    //qDebug() << Q_FUNC_INFO;
    if (hApp->gameRunning() and n > 0) {
        SleepFuncs::msleep(1000 / n);
        QMetaObject::invokeMethod(hFrame, "updateGameScreen", INVOKE_BLOCK, Q_ARG(bool, false));
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
void
hugo_init_screen( void )
{
    qRegisterMetaType<char*>("char*");
    qRegisterMetaType<HUGO_FILE>("HUGO_FILE");
    qRegisterMetaType<int*>("int*");
    waiterMutex = new QMutex;
    scriptBuffer = new QString;
    scrollbackBuffer = new QByteArray;
}


/* Returns true if the current display is capable of graphics display;
 * returns 2 if graphics are being routed to an external window other
 * than the main display. */
int
hugo_hasgraphics( void )
{
    if (hApp->settings()->enableGraphics) {
        return 1;
    }
    return false;
}


void
hugo_setgametitle( char* t )
{
    QMetaObject::invokeMethod(hMainWin, "setWindowTitle", INVOKE_BLOCK,
                              Q_ARG(QString, QString::fromLatin1(t)));
}


/* Does whatever has to be done to clean up the display pre-termination.
 */
void
hugo_cleanup_screen( void )
{
    delete waiterMutex;
    delete scriptBuffer;
    delete scrollbackBuffer;
}


/* Clears everything on the screen, moving the cursor to the top-left
 * corner of the screen.
 */
void
hugo_clearfullscreen( void )
{
    QMetaObject::invokeMethod(hHandlers, "clearfullscreen", INVOKE_BLOCK);
    currentpos = 0;
    currentline = 1;
    TB_Clear(0, 0, screenwidth, screenheight);
}


/* Clears the currently defined window, moving the cursor to the top-left
 * corner of the window.
 */
void
hugo_clearwindow( void )
{
    QMetaObject::invokeMethod(hHandlers, "clearwindow", INVOKE_BLOCK);
    currentpos = 0;
    currentline = 1;
    TB_Clear(physical_windowleft, physical_windowtop,
             physical_windowright, physical_windowbottom);
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
void
hugo_settextmode( void )
{
    QMetaObject::invokeMethod(hHandlers, "settextmode", INVOKE_BLOCK);
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
void
hugo_settextwindow( int left, int top, int right, int bottom )
{
    QMetaObject::invokeMethod(hHandlers, "settextwindow", INVOKE_BLOCK, Q_ARG(int, left),
                              Q_ARG(int, top), Q_ARG(int, right), Q_ARG(int, bottom));
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
void
hugo_settextpos( int x, int y )
{
    // Must be set:
    currentline = y;
    currentpos = (x - 1) * ::charwidth;   // Note:  zero-based

    // current_text_x/row are calculated assuming that the
    // character position (1, 1) is the pixel position (0, 0)
    current_text_x = physical_windowleft + currentpos;
    current_text_y = physical_windowtop + (y - 1) * lineheight;
}


/* PRINTFATALERROR may be #defined in heheader.h.
 */
void
printFatalError( char* a )
{
    QMetaObject::invokeMethod(hHandlers, "printFatalError", INVOKE_BLOCK,
                              Q_ARG(char*, a));
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
void
hugo_print( char* a )
{
    QMetaObject::invokeMethod(hHandlers, "print", INVOKE_BLOCK, Q_ARG(char*, a));
}


/* Scroll the current text window up one line.
 */
void
hugo_scrollwindowup()
{
    QMetaObject::invokeMethod(hFrame, "scrollUp", INVOKE_BLOCK, Q_ARG(int, physical_windowleft),
                              Q_ARG(int, physical_windowtop), Q_ARG(int, physical_windowright),
                              Q_ARG(int, physical_windowbottom), Q_ARG(int, lineheight));
    TB_Scroll();
}


/* The <f> argument is a mask containing any or none of:
   BOLD_FONT, UNDERLINE_FONT, ITALIC_FONT, PROP_FONT.

   If charwidth and lineheight change with a font change, these must be
   reset here as well.
*/
void
hugo_font( int f )
{
    QMetaObject::invokeMethod(hHandlers, "font", INVOKE_BLOCK, Q_ARG(int, f));
}


void
hugo_settextcolor( int c )
{
    QMetaObject::invokeMethod(hHandlers, "settextcolor", INVOKE_BLOCK, Q_ARG(int, c));
}


void
hugo_setbackcolor( int c )
{
    QMetaObject::invokeMethod(hHandlers, "setbackcolor", INVOKE_BLOCK, Q_ARG(int, c));
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
int
hugo_charwidth( char a )
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


int
hugo_textwidth( char* a )
{
    // With a fixed-width font, we know the width of the string is equal
    // to its length times the width of a character (all chars have the
    // same width.)
    if (not (currentfont & PROP_FONT)) {
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


int
hugo_strlen( char* a )
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


int
hugo_displaypicture( HUGO_FILE infile, long len )
{
    int result;
    QMetaObject::invokeMethod(hHandlers, "displaypicture", INVOKE_BLOCK,
                              Q_ARG(HUGO_FILE, infile), Q_ARG(long, len), Q_ARG(int*, &result));
    return result;
}


int
hugo_playmusic( HUGO_FILE infile, long reslength, char loop_flag )
{
    int result;
    QMetaObject::invokeMethod(hHandlers, "playmusic", INVOKE_BLOCK,
                              Q_ARG(HUGO_FILE, infile), Q_ARG(long, reslength),
                              Q_ARG(char, loop_flag), Q_ARG(int*, &result));
    return result;
}


void
hugo_musicvolume( int vol )
{
    QMetaObject::invokeMethod(hHandlers, "musicvolume", INVOKE_BLOCK, Q_ARG(int, vol));
}


void
hugo_stopmusic( void )
{
    QMetaObject::invokeMethod(hHandlers, "stopmusic", INVOKE_BLOCK);
}


int
hugo_playsample( HUGO_FILE infile, long reslength, char loop_flag )
{
    int result;
    QMetaObject::invokeMethod(hHandlers, "playsample", INVOKE_BLOCK,
                              Q_ARG(HUGO_FILE, infile), Q_ARG(long, reslength),
                              Q_ARG(char, loop_flag), Q_ARG(int*, &result));
    return result;
}


void
hugo_samplevolume( int vol )
{
    QMetaObject::invokeMethod(hHandlers, "samplevolume", INVOKE_BLOCK, Q_ARG(int, vol));
}


void
hugo_stopsample( void )
{
    QMetaObject::invokeMethod(hHandlers, "stopsample", INVOKE_BLOCK);
}


#ifdef DISABLE_VIDEO

void
muteVideo(bool)
{ }

void
updateVideoVolume()
{ }

int
hugo_hasvideo( void )
{ return false; }

void
hugo_stopvideo( void )
{ }

int
hugo_playvideo( HUGO_FILE infile, long, char, char, int )
{
    fclose(infile);
    return true;
}

#else

int
hugo_hasvideo( void )
{
    if (hApp->settings()->enableVideo and not hApp->settings()->videoSysError) {
        return true;
    }
    return false;
}


void
hugo_stopvideo( void )
{
    QMetaObject::invokeMethod(hHandlers, "stopvideo", INVOKE_BLOCK);
}


int
hugo_playvideo( HUGO_FILE infile, long len, char loop, char bg, int vol )
{
    int result;
    QMetaObject::invokeMethod(hHandlers, "playvideo", INVOKE_BLOCK,
                              Q_ARG(HUGO_FILE, infile), Q_ARG(long, len), Q_ARG(char, loop),
                              Q_ARG(char, bg), Q_ARG(int, vol), Q_ARG(int*, &result));
    return result;
}

#endif
