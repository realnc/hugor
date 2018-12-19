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
#pragma once
#include <QWidget>
#include <QQueue>
#include <QList>
#include <QFontMetrics>
#include <QMutex>
#include <QWaitCondition>

#include "happlication.h"

class HFrame;
class QMenu;
class QTimer;

extern HFrame* hFrame;

class HFrame final: public QWidget {
    Q_OBJECT

  private:
    // These values specify the exact input-mode we are in.
    enum class InputMode {
        // We aren't in input-mode. We still enqueue key presses though,
        // so they can be retrieved with getNextKey().
        None,

        // Return-terminated input.
        Normal
    };

    // The input-mode we are currently in.
    InputMode fInputMode = InputMode::None;

    // We have a finished user input.
    bool fInputReady = false;

    // Keypress input queue. If an element is 0, it means the input was
    // a mouse click.
    QQueue<char> fKeyQueue;

    // Mouse click input queue.
    QQueue<QPoint> fClickQueue;

    // The keypress and click queues are also accessed from the engine thread.
    QMutex fKeyQueueMutex;
    QMutex fClickQueueMutex;

    // Input buffer.
    QString fInputBuf;

    // Position at which we started reading input.
    int fInputStartX = 0;
    int fInputStartY = 0;

    // Current editor position, in characters. 0 is the start of the
    // current input buffer string.
    int fInputCurrentChar = 0;

    // Command history buffer.
    QList<QString> fHistory;

    // Maximum history capacity.
    int fMaxHistCap = 200;

    // Current command index in the history. 0 is the most recent one.
    int fCurHistIndex = 0;

    // Backup of the current, yet to be committed to history, input line.
    // We need this to back up the current input when the user recalls a
    // previous command from the history.
    QString fInputBufBackup;

    // Current colors.
    int fFgColor = 16;
    int fBgColor = 17;

    // Current font attributes.
    bool fUseFixedFont = false;
    bool fUseUnderlineFont = false;
    bool fUseItalicFont = false;
    bool fUseBoldFont = false;

    // Current font metrics.
    QFontMetrics fFontMetrics {QFont()};

    // We render game output into a pixmap first instead or painting directly
    // on the widget. We then draw the pixmap in our paintEvent().
    QPixmap fPixmap {1, 1};

    // We buffer text printed with printText() so that we can draw
    // whole strings rather than single characters at a time.
    QString fPrintBuffer;

    // Position at which to draw the buffered string.
    int fFlushXPos = 0;
    int fFlushYPos = 0;

    // Position of the text cursor.
    QPoint fCursorPos {0, 0};

    // Height of the text cursor in pixels.
    unsigned fHeight;

    // Last position of the text cursor.
    QPoint fLastCursorPos {0, 0};

    // Is the text cursor visible?
    bool fCursorVisible = false;

    // Text cursor blink visibility.  Changed by a timer to show/hide the
    // cursor at specific intervals if fCursorVisible is true.
    bool fBlinkVisible = false;

    // Text cursor blink timer.
    QTimer* fBlinkTimer;

    // Keeps track of whether the game screen needs updating.
    bool fNeedScreenUpdate = false;

    // We need a small time delay before minimizing when losing focus while
    // in fullscreen mode.
    QTimer* fMinimizeTimer;

    // Add a keypress to our input queue.
    void
    fEnqueueKey(char key, QMouseEvent* e);

  private slots:
    // Called by the timer to blink the text cursor.
    void
    fBlinkCursor();

    // We need to know when the application loses focus entirely so that we
    // can disable keyboard cursor blinking when we lose focus.
    void
    fHandleFocusChange( QWidget* old, QWidget* now );

    void
    fHandleFocusLost();

    // End line input mode and send the command to the game.
    void
    fEndInputMode( bool addToHistory );

  protected:
    void
    paintEvent(QPaintEvent* e) override;

    void
    resizeEvent( QResizeEvent* e ) override;

    void
    keyPressEvent( QKeyEvent* e ) override;

    void
    inputMethodEvent( QInputMethodEvent* e ) override;

    void
    singleKeyPressEvent( QKeyEvent* event );

    void
    mousePressEvent( QMouseEvent* e ) override;

    void
    mouseDoubleClickEvent( QMouseEvent* e ) override;

    void
    mouseMoveEvent( QMouseEvent* e ) override;

  signals:
    // Emitted when scrolling or paging up.
    void requestScrollback();

    // Emitted when the escape key is pressed on the keyboard.
    void escKeyPressed();

  public:
    HFrame( QWidget* parent );

    // The engine thread waits on this until an input line has been entered.
    QWaitCondition inputLineWaitCond;

    // The engine thread waits on this until a keypress is available.
    QWaitCondition keypressAvailableWaitCond;

    // Start reading an input line.
    void
    startInput( int xPos, int yPos );

    // Get the most recently entered input line and clear it.
    void
    getInput(char* buf, size_t buflen);

    // Returns the next character waiting in the queue. If the queue is
    // empty, it will wait for a character to become available.
    // Note: if this returns 0, it means the next "key" is a mouse click;
    // call getNextClick() to get the position of the mouse click.
    int
    getNextKey();

    QPoint
    getNextClick();

    bool
    hasKeyInQueue();

    // Clear a region of the window using the current background color.
    void
    clearRegion( int left, int top, int right, int bottom );

    // Set the current foreground color.
    void
    setFgColor( int color );

    // Set the current background color.
    void
    setBgColor( int color );

    void
    setFontType( int hugoFont );

    const QFontMetrics&
    currentFontMetrics() const
    { return this->fFontMetrics; }

    // Print text using the current foreground and background colors.
    // The Text is might not be printed immediately; call flushText()
    // to flush the accumulated text to the screen.
    void
    printText( const QString& str, int x, int y );

    // Print an image to the screen. The image is printed immediately
    // (no buffering is performed.)
    void
    printImage( const QImage& img, int x, int y );

    // Change the text cursor position.
    void
    moveCursorPos( const QPoint& pos )
    { this->fCursorPos = pos; }

    // Set the height of the text cursor in pixels.
    void
    setCursorHeight( unsigned height )
    { this->fHeight = height; }

    // Show/hide the text cursor.
    void
    setCursorVisible( bool visible )
    { this->fCursorVisible = visible; }

    bool
    isCursorVisible() const
    { return this->fCursorVisible; }

    void
    updateCursorPos();

    // Reset cursor blink timer.  This will read the blinking rate from the
    // desktop environment and ajust the blink timer as needed.
    void
    resetCursorBlinking();

    // Insert text at the current command input position. If 'execute' is set,
    // the command is also executed. If 'clear' is set, the current command
    // will be cleared before adding the text.
    void
    insertInputText( QString txt, bool execute, bool clearCurrent );

    // Returns a list of current context menu entries set by the game
    // and inserts them into the `dst` menu.
    QList<const QAction*>
    getGameContextMenuEntries( QMenu& dst );

  public slots:
    // Flush any pending text drawing.
    void
    flushText();

    // Scroll a region of the screen up by 'h' pixels.
    void
    scrollUp( int left, int top, int right, int bottom, int h );

    // Update the game screen, if needed.
    void
    updateGameScreen(bool force);
};
