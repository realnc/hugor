#ifndef HFRAME_H
#define HFRAME_H

#include <QWidget>
#include <QQueue>
#include <QList>
#include <QFontMetrics>

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

    // Current font attributes.
    bool fUseFixedFont;
    bool fUseUnderlineFont;
    bool fUseItalicFont;
    bool fUseBoldFont;

    // Current font metrics.
    QFontMetrics fFontMetrics;

    // We use a pixmap to render to, and then display that in our
    // paintEvent().
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

  private slots:
    // Called by the timer to blink the text cursor.
    void
    fBlinkCursor();

    // We need to know when the application loses focus entirely so that we
    // can disable keyboard cursor blinking when we lose focus.
    void
    fHandleFocusChange( QWidget* old, QWidget* now );

  protected:
    virtual void
    paintEvent( QPaintEvent* );

    virtual void
    resizeEvent( QResizeEvent* e );

    virtual void
    keyPressEvent( QKeyEvent* e );

    virtual void
    inputMethodEvent( QInputMethodEvent* e );

    void
    singleKeyPressEvent( QKeyEvent* event );

    void
    mousePressEvent( QMouseEvent* e );

    void
    mouseDoubleClickEvent( QMouseEvent* e );

  signals:
    // Emitted when an input operation has finished successfully.
    void inputReady();

    // Emitted when scrolling or paging up.
    void requestScrollback();

  public:
    HFrame( QWidget* parent );

    // Read an input line.
    void
    getInput( char* buf, size_t buflen, int xPos, int yPos );

    // Returns the next character waiting in the queue. If the queue is
    // empty, it will wait for a character to become available.
    // Note: if this returns 0, it means the next "key" is a mouse click;
    // call getNextClick() to get the position of the mouse click.
    int
    getNextKey();

    QPoint
    getNextClick();

    bool
    hasKeyInQueue()
    {
        this->flushText();
        return not this->fKeyQueue.isEmpty();
    }

    // Clear a region of the window using the current background color.
    void
    clearRegion( int left, int top, int right, int bottom );

    // Set the current foreground color.
    void
    setFgColor( int color )
    {
        this->flushText();
        this->fFgColor = color;
    }

    // Set the current background color.
    void
    setBgColor( int color )
    {
        this->flushText();
        this->fBgColor = color;
        hApp->updateMargins(this->fBgColor);
    }

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

    // Scroll a region of the screen up by 'h' pixels.
    void
    scrollUp( int left, int top, int right, int bottom, int h );

    // Flush any pending text drawing.
    void
    flushText();

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
};


#endif // HFRAME_H
