#include <SDL.h>
#include <SDL_mixer.h>
#include <QDebug>
#include <QFontMetrics>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QFileDialog>

#include "happlication.h"
#include "hmainwindow.h"
#include "hframe.h"
#include "hwindow.h"
#include "settings.h"
#include "hugodefs.h"

extern "C" {
#include "heheader.h"
}

/* Since we provide our own command history: */
//#define HISTORY_SIZE 16
//int hcount = 0;
//char* history[HISTORY_SIZE];


/* Helper routine. Converts a Hugo color to a Qt color.
 */
QColor
hugoColorToQt( int color )
{
    QColor qtColor;
    switch (color) {
      case 16:                 qtColor = hApp->settings()->mainTextColor; break;
      case 17:                 qtColor = hApp->settings()->mainBgColor; break;
      case 18:                 qtColor = hApp->settings()->statusTextColor; break;
      case 19:                 qtColor = hApp->settings()->statusBgColor; break;
      case 20:                 color = fcolor; // Fall-through
      case HUGO_BLACK:         qtColor.setRgb(0xff000000); break;
      case HUGO_BLUE:          qtColor.setRgb(0xff00007f); break;
      case HUGO_GREEN:         qtColor.setRgb(0xff007f00); break;
      case HUGO_CYAN:          qtColor.setRgb(0xff007f7f); break;
      case HUGO_RED:           qtColor.setRgb(0xff7f0000); break;
      case HUGO_MAGENTA:       qtColor.setRgb(0xff7f007f); break;
      case HUGO_BROWN:         qtColor.setRgb(0xff7f5f00); break;
      case HUGO_WHITE:         qtColor.setRgb(0xffcfcfcf); break;
      case HUGO_DARK_GRAY:     qtColor.setRgb(0xff3f3f3f); break;
      case HUGO_LIGHT_BLUE:    qtColor.setRgb(0xff0000ff); break;
      case HUGO_LIGHT_GREEN:   qtColor.setRgb(0xff00ff00); break;
      case HUGO_LIGHT_CYAN:    qtColor.setRgb(0xff00ffff); break;
      case HUGO_LIGHT_RED:     qtColor.setRgb(0xffff0000); break;
      case HUGO_LIGHT_MAGENTA: qtColor.setRgb(0xffff00ff); break;
      case HUGO_YELLOW:        qtColor.setRgb(0xffffff00); break;
      case HUGO_BRIGHT_WHITE:  qtColor.setRgb(0xffffffff); break;
      default:                 qtColor.setRgb(0xff000000);
    }
    return qtColor;
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
    qstrcpy(drive, "");
    qstrcpy(dir, "");
    qstrcpy(fname, "");
    qstrcpy(ext, "");

    QFileInfo inf(QString::fromLocal8Bit(path));
    qstrcpy(ext, inf.suffix().toLocal8Bit().constData());
    qstrcpy(fname, inf.completeBaseName().toLocal8Bit().constData());
    qstrcpy(dir, inf.absolutePath().toLocal8Bit().constData());
    //qDebug() << "splitpath:" << drive << dir << fname << ext;
    return;
}

void
hugo_makepath( char* path, char* drive, char* dir, char* fname, char* ext )
{
    if (*ext == '.') ext++;
    strcpy(path,drive);
    strcat(path,dir);
    switch (*(path+strlen(path)))
    {
    case '/':
    case ':':
        /*        case 0: */
        break;
    default:
        if (strcmp(path, "")) strcat(path,"/");
        break;
    }
    strcat(path,fname);
    if (strcmp(ext, "")) strcat(path,".");
    char* tmp = new char[strlen(ext)];
    strcpy(tmp, ext);
    strcat(path,strlwr(tmp));
    delete[] tmp;
    //qDebug() << "result:" << path;
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
    Q_ASSERT(a != 0 and b != 0);

    QString fname;
    const QString& filter = QObject::tr("Hugo Saved Games")
                            + QString::fromAscii(" (*.sav *.Sav *.SAV)");
    if (QString::fromAscii(a).endsWith("save")) {
        fname = QFileDialog::getSaveFileName(hMainWin, "Save current game", b, filter);
    } else {
        fname = QFileDialog::getOpenFileName(hMainWin, "Restore a saved game", b, filter);
    }

    qstrcpy(line, fname.toLocal8Bit().constData());
    return;
}


/* hugo_overwrite

    Checks to see if the given filename already exists, and prompts to
    replace it.  Returns true if file may be overwritten.

    Again, it may be preferable to replace this with something fancier.
*/
int
hugo_overwrite( char* f )
{
    // We handle the overwrite confirmation in hugo_getfilename().
    return true;
}


/* hugo_closefiles

    Closes all open files.  NOTE:  If the operating system automatically
    closes any open streams upon exit from the program, this function may
    be left empty.
*/
void
hugo_closefiles()
{
    fclose(game);
    if (script)
        fclose(script);
    if (io)
        fclose(io);
    if (record)
        fclose(record);
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
    //qDebug() << "getkey";
    //hFrame->moveCursorPos(QPoint(current_text_x, current_text_y));
    int c = hFrame->getNextKey();
    if (c == 0) {
        // It's a mouse click.
        const QPoint& pos = hFrame->getNextClick();
        display_pointer_x = (pos.x() - physical_windowleft) / FIXEDCHARWIDTH + 1;
        display_pointer_y = (pos.y() - physical_windowtop) / FIXEDLINEHEIGHT + 1;
        return 1;
    }
    return c;
}


/* hugo_getline

    Gets a line of input from the keyboard, storing it in <buffer>.
*/
void
hugo_getline( char* p )
{
    hugo_print(p);
    hFrame->setCursorVisible(true);
    hFrame->moveCursorPos(QPoint(current_text_x, current_text_y));
    hFrame->getInput(buffer, MAXBUFFER, current_text_x, current_text_y);
    hFrame->setCursorVisible(false);
    hugo_print(const_cast<char*>("\r\n"));
}


/* hugo_sendtoscrollback

   Stores a given line in the scrollback buffer (optional).
*/
/*
void
hugo_sendtoscrollback( char* a )
{ }
*/


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
    hApp->advanceEventLoop();
    return hFrame->hasKeyInQueue();
}


/* hugo_timewait

    Waits for 1/n seconds.  Returns false if waiting is unsupported.
*/
int
hugo_timewait( int n )
{
    if (not hApp->gameRunning() or n < 1) {
        return false;
    }

    QEventLoop idleLoop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &idleLoop, SLOT(quit()));
    QObject::connect(hApp, SIGNAL(gameQuitting()), &idleLoop, SLOT(quit()));
    timer.start(1000 / n);
    idleLoop.exec();
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
}


/* Returns true if the current display is capable of graphics display;
 * returns 2 if graphics are being routed to an external window other
 * than the main display. */
int
hugo_hasgraphics( void )
{
    if (hApp->settings()->enableGraphics)
        return true;
    return false;
}


void
hugo_setgametitle( char* t )
{
    hMainWin->setWindowTitle(QString::fromLatin1(t));
}


/* Does whatever has to be done to clean up the display pre-termination.
 */
void
hugo_cleanup_screen( void )
{
}


/* Clears everything on the screen, moving the cursor to the top-left
 * corner of the screen.
 */
void
hugo_clearfullscreen( void )
{
    hFrame->clearRegion(0, 0, 0, 0);
    currentpos = 0;
    currentline = 1;
}


/* Clears the currently defined window, moving the cursor to the top-left
 * corner of the window.
 */
void
hugo_clearwindow( void )
{
    hFrame->setBgColor(bgcolor);
    hFrame->clearRegion(physical_windowleft, physical_windowtop,
                        physical_windowright, physical_windowbottom);
    currentpos = 0;
    currentline = 1;
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
    const QFontMetrics& curMetr = hFrame->currentFontMetrics();
    const QFontMetrics fixedMetr(hApp->settings()->fixedFont);

    FIXEDCHARWIDTH = fixedMetr.averageCharWidth();
    FIXEDLINEHEIGHT = fixedMetr.height();

    charwidth = curMetr.averageCharWidth();
    lineheight = curMetr.height();

    SCREENWIDTH = hFrame->width();
    SCREENHEIGHT = hFrame->height();

    /* Must be set: */
    hugo_settextwindow(1, 1,
        SCREENWIDTH/FIXEDCHARWIDTH, SCREENHEIGHT/FIXEDLINEHEIGHT);
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
    //qDebug() << "settextwindow" << left << top << right << bottom;
    /* Must be set: */
    physical_windowleft = (left - 1) * FIXEDCHARWIDTH;
    physical_windowtop = (top - 1) * FIXEDLINEHEIGHT;
    physical_windowright = right * FIXEDCHARWIDTH - 1;
    physical_windowbottom = bottom * FIXEDLINEHEIGHT - 1;

    // Correct for full-width windows where the right border would
    // otherwise be clipped to a multiple of charwidth, leaving a
    // sliver of the former window at the righthand side.
    if (right >= SCREENWIDTH / FIXEDCHARWIDTH)
        physical_windowright = hFrame->width() - 1;
    if (bottom >= SCREENHEIGHT / FIXEDLINEHEIGHT)
        physical_windowbottom = hFrame->height() - 1;

    physical_windowwidth = physical_windowright - physical_windowleft + 1;
    physical_windowheight = physical_windowbottom - physical_windowtop + 1;

    hFrame->setFgColor(fcolor);
    hFrame->setBgColor(bgcolor);
    hFrame->setFontType(currentfont);
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
    currentpos = (x - 1) * charwidth;   // Note:  zero-based

    // current_text_x/row are calculated assuming that the
    // character position (1, 1) is the pixel position (0, 0)
    current_text_x = physical_windowleft + currentpos;
    current_text_y = physical_windowtop + (y - 1) * lineheight;
}


/* PRINTFATALERROR may be #defined in heheader.h.
 */
extern "C" void
printFatalError( char* a )
{
    hugo_print(a);
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
    //const QFontMetrics& m = hFrame->currentFontMetrics();
    uint len = qstrlen(a);
    QByteArray ac;

    for (uint i = 0; i < len; ++i) {
        // If we've passed the bottom of the window, align to the bottom edge.
        if (current_text_y > physical_windowbottom - lineheight) {
            int temp_lh = lineheight;
            lineheight = current_text_y - physical_windowbottom + lineheight;
            current_text_y -= lineheight;
            if (inwindow)
                --lineheight;
            hugo_scrollwindowup();
            lineheight = temp_lh;
        }

        switch (a[i]) {
          case '\n':
            hFrame->flushText();
            current_text_y += lineheight;
            hFrame->update();
            //hApp->advanceEventLoop();
            //last_was_italic = false;
            break;

          case '\r':
            hFrame->flushText();
            if (!inwindow) {
                current_text_x = physical_windowleft - FIXEDCHARWIDTH;
            } else {
                current_text_x = 0;
            }
            current_text_x = physical_windowleft;
            //last_was_italic = false;
            break;

          default: {
            ac += a[i];
            //hFrame->printText(QString(QChar(a[i])).toAscii().constData(), current_text_x, current_text_y);
            //hFrame->flushText();
            //current_text_x += hugo_charwidth(a[i]);
          }
        }
    }

    hFrame->printText(ac.constData(), current_text_x, current_text_y);
    current_text_x += hFrame->currentFontMetrics().width(QString::fromAscii(ac.constData()));
    //qDebug() << printBuf;

    // Check again after printing.
    if (current_text_y > physical_windowbottom - lineheight) {
        int temp_lh = lineheight;
        hFrame->flushText();
        lineheight = current_text_y - physical_windowbottom + lineheight;
        current_text_y -= lineheight;
        if (inwindow)
            --lineheight;
        //tweaking_lineheight = true;
        hugo_scrollwindowup();
        //tweaking_lineheight = false;
        lineheight = temp_lh;
    }
    hFrame->update();
}


/* Scroll the current text window up one line.
 */
void
hugo_scrollwindowup()
{
    int h = hFrame->currentFontMetrics().height();
    hFrame->scrollUp(physical_windowleft, physical_windowtop, physical_windowright, physical_windowbottom, h);
}


/* The <f> argument is a mask containing any or none of:
   BOLD_FONT, UNDERLINE_FONT, ITALIC_FONT, PROP_FONT.

   If charwidth and lineheight change with a font change, these must be
   reset here as well.
*/
void
hugo_font( int f )
{
    hFrame->setFontType(f);
    charwidth = hFrame->currentFontMetrics().averageCharWidth();
    lineheight = hFrame->currentFontMetrics().height();
}


void
hugo_settextcolor( int c )
{
    hFrame->setFgColor(c);
}


void
hugo_setbackcolor( int c )
{
    hFrame->setBgColor(c);
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
        return hFrame->currentFontMetrics().width(a);
    }
    return FIXEDCHARWIDTH;
}


int
hugo_textwidth( char* a )
{
    size_t slen = qstrlen(a);
    QByteArray str;

    // Construct a string that contains only printable characters.
    for (size_t i = 0; i < slen; ++i) {
        if (a[i] == COLOR_CHANGE)
            i += 2;
        else if (a[i] == FONT_CHANGE)
                ++i;
        else
            str += a[i];
    }
    return hFrame->currentFontMetrics().width(QString::fromAscii(str));
}


int
hugo_strlen( char* a )
{
    size_t len = 0;
    size_t slen = qstrlen(a);

    for (size_t i = 0; i < slen; ++i) {
        if (a[i] == COLOR_CHANGE)
            i += 2;
        else if (a[i] == FONT_CHANGE)
            ++i;
        else
            ++len;
    }
    return len;
}


// FIXME: Check for errors when loading images.
extern "C" int
hugo_displaypicture( FILE* infile, long len )
{
    // Open it as a QFile.
    long pos = ftell(infile);
    QFile file;
    file.open(infile, QIODevice::ReadOnly);
    file.seek(pos);

    // Load the data into a byte array.
    const QByteArray& data = file.read(len);

    // Create the image from the data.
    // FIXME: Allow only JPEG images. By default, QImage supports
    // all image formats recognized by Qt.
    QImage img;
    img.loadFromData(data);

    // Done with the file.
    file.close();
    fclose(infile);

    // Scale the image, if needed.
    QSize imgSize(img.size());
    if (img.width() > physical_windowwidth) {
        imgSize.setWidth(physical_windowwidth);
    }
    if (img.height() > physical_windowheight) {
        imgSize.setHeight(physical_windowheight);
    }
    // Make sure to keep the aspect ratio (don't stretch.)
    if (imgSize != img.size()) {
        // Only apply a smoothing filter if that setting is enabled
        // in the settings.
        Qt::TransformationMode mode;
        if (hApp->settings()->useSmoothScaling) {
            mode = Qt::SmoothTransformation;
        } else {
            mode = Qt::FastTransformation;
        }
        img = img.scaled(imgSize, Qt::KeepAspectRatio, mode);
        imgSize = img.size();
    }

    // The image should be displayed centered.
    int x = (physical_windowwidth - imgSize.width()) / 2 + physical_windowleft;
    int y = (physical_windowheight - imgSize.height()) / 2 + physical_windowtop;
    hFrame->printImage(img, x, y);
    hFrame->update();
    return true;
}


extern "C" int
hugo_playmusic( HUGO_FILE infile, long reslength, char loop_flag )
{
    if (not hApp->settings()->enableMusic) {
        fclose(infile);
        return false;
    }

    // We only play one music track at a time, so it's enough
    // to make these static.
    static QFile* file = 0;
    static Mix_Music* music = 0;

    // Any currently playing music should be stopped before playing
    // a new one.
    Mix_HaltMusic();

    // If a file already exists from a previous call, delete it first.
    if (file != 0) {
        delete file;
        file = 0;
    }

    // Open 'infile' as a QFile.
    file = new QFile;
    if (not file->open(infile, QIODevice::ReadOnly)) {
        qWarning() << "ERROR: Can't open game music file";
        file->close();
        fclose(infile);
        return false;
    }

    // Map the data into memory and create an RWops from that data.
    SDL_RWops* rwops = SDL_RWFromConstMem(file->map(ftell(infile), reslength), reslength);
    // Done with the file.
    file->close();
    fclose(infile);
    if (rwops == 0) {
        qWarning() << "ERROR:" << SDL_GetError();
        return false;
    }

    // If a Mix_Music* already exists from a previous call, delete it first.
    if (music != 0) {
        Mix_FreeMusic(music);
        music = 0;
    }

    // Create a Mix_Music* from the RWops. Mix_LoadMUS_RW() takes ownership
    // of the RWops; it will free it as necessary.
    music = Mix_LoadMUS_RW(rwops);
    if (music == 0) {
        qWarning() << "ERROR:" << Mix_GetError();
        return false;
    }

    // Start playing the music. Loop forever if 'loop_flag' is true.
    // Otherwise, just play it once.
    if (Mix_PlayMusic(music, loop_flag ? -1 : 1) != 0) {
        qWarning() << "ERROR:" << Mix_GetError();
        Mix_FreeMusic(music);
        return false;
    }
    return true;
}

extern "C" void
hugo_musicvolume( int vol )
{
    if (vol < 0)
        vol = 0;
    else if (vol > 100)
        vol = 100;

    // Convert the Hugo volume range [0..100] to the SDL volume
    // range [0..MIX_MAX_VOLUME].
    vol = (vol * MIX_MAX_VOLUME) / 100;
    Mix_VolumeMusic(vol);
}

extern "C" void
hugo_stopmusic( void )
{
    Mix_HaltMusic();
}

extern "C" int
hugo_playsample( HUGO_FILE infile, long reslength, char loop_flag )
{
    if (not hApp->settings()->enableSoundEffects) {
        fclose(infile);
        return false;
    }

    // We only play one sample at a time, so it's enough to make these
    // static.
    static QFile* file = 0;
    static Mix_Chunk* chunk = 0;

    // Any currently playing sample should be stopped before playing
    // a new one.
    Mix_HaltChannel(-1);

    // If a file already exists from a previous call, delete it first.
    if (file != 0) {
        delete file;
        file = 0;
    }

    // Open 'infile' as a QFile.
    file = new QFile;
    if (not file->open(infile, QIODevice::ReadOnly)) {
        qWarning() << "ERROR: Can't open sample sound file";
        file->close();
        fclose(infile);
        return false;
    }

    // Map the data into memory and create an RWops from that data.
    SDL_RWops* rwops = SDL_RWFromConstMem(file->map(ftell(infile), reslength), reslength);
    // Done with the file.
    file->close();
    fclose(infile);
    if (rwops == 0) {
        qWarning() << "ERROR:" << SDL_GetError();
        return false;
    }

    // If a Mix_Chunk* already exists from a previous call, delete it first.
    if (chunk != 0) {
        Mix_FreeChunk(chunk);
        chunk = 0;
    }

    // Create a Mix_Chunk* from the RWops. Tell Mix_LoadWAV_RW() to take
    // ownership of the RWops so it will free it as necessary.
    chunk = Mix_LoadWAV_RW(rwops, true);
    if (chunk == 0) {
        qWarning() << "ERROR:" << Mix_GetError();
        return false;
    }

    // Start playing the sample. Loop forever if 'loop_flag' is true.
    // Otherwise, just play it once.
    if (Mix_PlayChannel(-1, chunk, loop_flag ? -1 : 0) < 0) {
        qWarning() << "ERROR:" << Mix_GetError();
        Mix_FreeChunk(chunk);
        return false;
    }
    return true;
}

extern "C" void
hugo_samplevolume( int vol )
{
    if (vol < 0)
        vol = 0;
    else if (vol > 100)
        vol = 100;

    // Convert the Hugo volume range [0..100] to the SDL volume
    // range [0..MIX_MAX_VOLUME].
    vol = (vol * MIX_MAX_VOLUME) / 100;
    Mix_Volume(-1, vol);
}

extern "C" void
hugo_stopsample( void )
{
    Mix_HaltChannel(-1);
}


#if !defined (COMPILE_V25)
int
hugo_hasvideo( void )
{
    return false;
}

extern "C" void
hugo_stopvideo( void )
{ }

extern "C" int
hugo_playvideo( HUGO_FILE infile, long reslength, char loop_flag )
{
    fclose(infile);
    return true;    /* not an error */
}
#endif  /* !defined (COMPILE_V25) */
