#ifndef HFRAME_H
#define HFRAME_H

#include <QWidget>
#include <QQueue>
#include <QList>
#include <QFontMetrics>
#include <QMutex>
#include <QWaitCondition>

#include "happlication.h"


extern class HFrame* hFrame;


class HFrame: public QWidget {
    Q_OBJECT

  private:
    // These values specify the exact input-mode we are in.
    enum InputMode {
        // We aren't in input-mode. We still enqueue key presses though,
        // so they can be retrieved with getNextKey().
        NoInput,

        // Return-terminated input.
        NormalInput
    };

    // The input-mode we are currently in.
    InputMode fInputMode;

    // We have a finished user input.
    bool fInputReady;

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
    int fInputStartX;
    int fInputStartY;

    // Current editor position, in characters. 0 is the start of the
    // current input buffer string.
    int fInputCurrentChar;

    // Command history buffer.
    QList<QString> fHistory;

    // Maximum history capacity.
    int fMaxHistCap;

    // Current command index in the history. 0 is the most recent one.
    int fCurHistIndex;

    // Backup of the current, yet to be committed to history, input line.
    // We need this to back up the current input when the user recalls a
    // previous command from the history.
    QString fInputBufBackup;

    // Current colors.
    int fFgColor;
    int fBgColor;
    int fMarginColor;

    // Current font attributes.
    bool fUseFixedFont;
    bool fUseUnderlineFont;
    bool fUseItalicFont;
    bool fUseBoldFont;

    // Current font metrics.
    QFontMetrics fFontMetrics;

    // We render game output into a pixmap first instead or painting directly
    // on the widget. We then draw the pixmap in our paintEvent().
    QPixmap fPixmap;

    // We buffer text printed with printText() so that we can draw
    // whole strings rather than single characters at a time.
    QString fPrintBuffer;

    // Position at which to draw the buffered string.
    int fFlushXPos;
    int fFlushYPos;

    // Position of the text cursor.
    QPoint fCursorPos;

    // Height of the text cursor in pixels.
    unsigned fHeight;

    // Last position of the text cursor.
    QPoint fLastCursorPos;

    // Is the text cursor visible?
    bool fCursorVisible;

    // Text cursor blink visibility.  Changed by a timer to show/hide the
    // cursor at specific intervals if fCursorVisible is true.
    bool fBlinkVisible;

    // Text cursor blink timer.
    class QTimer* fBlinkTimer;

    // Mute timer.
    class QTimer* fMuteTimer;

    // Keeps track of whether the game screen needs updating.
    bool fNeedScreenUpdate;

    // Add a keypress to our input queue.
    void
    fEnqueueKey(char key);

  private slots:
    // Called by the timer to blink the text cursor.
    void
    fBlinkCursor();

    // Mute the sound backend.
    void
    fMuteSound();

    // We need to know when the application loses focus entirely so that we
    // can disable keyboard cursor blinking when we lose focus.
    void
    fHandleFocusChange( QWidget* old, QWidget* now );

    // End line input mode and send the command to the game.
    void
    fEndInputMode( bool addToHistory );

  protected:
    virtual void
    paintEvent(QPaintEvent* e);

    virtual void
    resizeEvent( QResizeEvent* e );

    virtual void
    keyPressEvent( QKeyEvent* e );

    virtual void
    inputMethodEvent( QInputMethodEvent* e );

    void
    singleKeyPressEvent( QKeyEvent* event );

    virtual void
    mousePressEvent( QMouseEvent* e );

    virtual void
    mouseDoubleClickEvent( QMouseEvent* e );

    virtual void
    mouseMoveEvent( QMouseEvent* e );

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
    isCursorVisible()
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
    getGameContextMenuEntries( class QMenu& dst );

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


#endif // HFRAME_H
