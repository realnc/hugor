#include <SDL_mixer.h>
#include <QDebug>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QClipboard>
#include <QPainter>
#include <QTimer>

extern "C" {
#include "heheader.h"
}
#include "hframe.h"
#include "happlication.h"
#include "hwindow.h"
#include "hugodefs.h"
#include "settings.h"


HFrame* hFrame = 0;


HFrame::HFrame( QWidget* parent )
    : QWidget(parent),
      fInputMode(NoInput),
      fInputReady(false),
      fInputStartX(0),
      fInputStartY(0),
      fInputCurrentChar(0),
      fMaxHistCap(200),
      fCurHistIndex(0),
      fFgColor(HUGO_BLACK),
      fBgColor(HUGO_WHITE),
      fUseFixedFont(false),
      fUseUnderlineFont(false),
      fUseItalicFont(false),
      fUseBoldFont(false),
      fFontMetrics(QFont()),
      fPixmap(1, 1),
      fFlushXPos(0),
      fFlushYPos(0),
      fCursorPos(0, 0),
      fLastCursorPos(0, 0),
      fCursorVisible(false),
      fBlinkVisible(false),
      fBlinkTimer(new QTimer(this))
{
    // We handle player input, so we need to accept focus.
    this->setFocusPolicy(Qt::WheelFocus);

    connect(this->fBlinkTimer, SIGNAL(timeout()), this, SLOT(fBlinkCursor()));
    this->resetCursorBlinking();
    //this->setCursorVisible(true);

    // Our initial height is the height of the current proportional font.
    this->fHeight = QFontMetrics(hApp->settings()->propFont).height();

    // We need to check whether the application lost focus.
    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), SLOT(fHandleFocusChange(QWidget*,QWidget*)));

    hFrame = this;
}


void
HFrame::fBlinkCursor()
{
    this->fBlinkVisible = not this->fBlinkVisible;
    this->update(this->fCursorPos.x(), this->fCursorPos.y(), this->fCursorPos.x() + 2,
                 this->fCursorPos.y() + this->fHeight + 1);
}


void
HFrame::fHandleFocusChange( QWidget* old, QWidget* now )
{
    if (now == 0) {
        if (hApp->settings()->muteSoundInBackground) {
            muteSound(true);
        }
        // The application window lost focus.  Disable cursor blinking.
        this->fBlinkTimer->stop();
#ifdef Q_WS_MAC
        // On the Mac, when applications lose focus the cursor must be disabled.
        if (this->fBlinkVisible) {
            this->fBlinkCursor();
        }
#else
        // On all other systems we assume the cursor must stay visible.
        if (not this->fBlinkVisible) {
            this->fBlinkCursor();
        }
#endif
    } else if (old == 0 and now != 0) {
        if (hApp->settings()->muteSoundInBackground) {
            muteSound(false);
        }
        // The application window gained focus.  Reset cursor blinking.
        this->resetCursorBlinking();
    }
}


void
HFrame::paintEvent( QPaintEvent* )
{
    QPainter p(this);
    p.drawPixmap(0, 0, this->fPixmap);

    // Draw our current input. We need to do this here, after the pixmap
    // has already been painted, so that the input gets painted on top.
    // Otherwise, we could not erase text during editing.
    if (this->fInputMode == NormalInput and not this->fInputBuf.isEmpty()) {
        QFont f(this->fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
        f.setUnderline(this->fUseUnderlineFont);
        f.setItalic(this->fUseItalicFont);
        f.setBold(this->fUseBoldFont);
        QFontMetrics m(f);
        p.setFont(f);
        p.setPen(hugoColorToQt(this->fFgColor));
        p.setBackgroundMode(Qt::OpaqueMode);
        p.setBackground(QBrush(hugoColorToQt(this->fBgColor)));
        p.drawText(this->fInputStartX, this->fInputStartY + m.ascent(), this->fInputBuf);
    }

    // Likewise, the input caret needs to be painted on top of the input text.
    if (this->fCursorVisible and this->fBlinkVisible) {
        p.setPen(hugoColorToQt(this->fFgColor));
        p.drawLine(this->fCursorPos.x(), this->fCursorPos.y(),
                   this->fCursorPos.x(), this->fCursorPos.y() + this->fHeight);
    }
}


void
HFrame::resizeEvent( QResizeEvent* e )
{
    // Save a copy of the current pixmap.
    const QPixmap& tmp = this->fPixmap.copy();

    // Create a new pixmap, using the new size and fill it with
    // the default background color.
    this->fPixmap = QPixmap(e->size());
    this->fPixmap.fill(hugoColorToQt(this->fBgColor));

    // Draw the saved pixmap into the new one.
    QPainter p(&this->fPixmap);
    p.drawPixmap(0, 0, tmp);

    hugo_settextmode();
    display_needs_repaint = true;
}


void
HFrame::keyPressEvent( QKeyEvent* e )
{
    //qDebug() << Q_FUNC_INFO;

    //qDebug() << "Key pressed:" << hex << e->key();

    if (not hApp->gameRunning()) {
        QWidget::keyPressEvent(e);
        return;
    }

    if (this->fInputMode == NoInput) {
        this->singleKeyPressEvent(e);
        return;
    }

    if (e->matches(QKeySequence::MoveToStartOfLine) or e->matches(QKeySequence::MoveToStartOfBlock)) {
        this->fInputCurrentChar = 0;
    } else if (e->matches(QKeySequence::MoveToEndOfLine) or e->matches(QKeySequence::MoveToEndOfBlock)) {
        this->fInputCurrentChar = this->fInputBuf.length();
#if QT_VERSION >= 0x040500
    } else if (e->matches(QKeySequence::InsertParagraphSeparator)) {
#else
    } else if (e->key() == Qt::Key_Enter or e->key() == Qt::Key_Return) {
#endif
        this->fInputReady = true;
        this->fInputMode = NoInput;
        // If we're about to overflow the max history cap, delete the
        // oldest command from the history.
        if (this->fHistory.size() > this->fMaxHistCap) {
            this->fHistory.removeFirst();
        }
        // If we were editing a recalled command (which means it's
        // already in the history), just reset the current history
        // index. Otherwise, if the command is not a duplicate of the
        // latest one in the history and it's not empty, commit it.
        if (this->fCurHistIndex > 0) {
            this->fCurHistIndex = 0;
        } else if (this->fHistory.isEmpty() or (this->fInputBuf != this->fHistory.last()
                   and not this->fInputBuf.isEmpty()))
        {
            this->fHistory.append(this->fInputBuf);
        }
        emit inputReady();
        return;
    } else if (e->matches(QKeySequence::Delete)) {
        //this->fTadsBuffer->del_right();
        if (this->fInputCurrentChar < this->fInputBuf.length()) {
            this->fInputBuf.remove(this->fInputCurrentChar, 1);
        }
    } else if (e->matches(QKeySequence::DeleteEndOfWord)) {
        //this->fTadsBuffer->move_right(true, true);
        //this->fTadsBuffer->del_selection();
    } else if (e->matches(QKeySequence::DeleteStartOfWord)) {
        //this->fTadsBuffer->move_left(true, true);
        //this->fTadsBuffer->del_selection();
    } else if (e->matches(QKeySequence::MoveToPreviousChar)) {
        if (this->fInputCurrentChar > 0)
            --this->fInputCurrentChar;
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        //this->fTadsBuffer->move_right(false, false);
        if (this->fInputCurrentChar < this->fInputBuf.length())
            ++this->fInputCurrentChar;
    } else if (e->matches(QKeySequence::MoveToPreviousWord)) {
        //this->fTadsBuffer->move_left(false, true);
    } else if (e->matches(QKeySequence::MoveToNextWord)) {
        //this->fTadsBuffer->move_right(false, true);
    } else if (e->matches(QKeySequence::MoveToPreviousLine)) {
        // If we're already at the oldest command in the history, or
        // the history list is empty, don't do anything.
        if (this->fCurHistIndex == this->fHistory.size() or this->fHistory.isEmpty()) {
            return;
        }
        // If the current command is new and not in the history yet,
        // remember it so we can bring it back if the user recalls it.
        if (this->fCurHistIndex == 0) {
            this->fInputBufBackup = this->fInputBuf;
        }
        // Recall the previous command from the history.
        this->fInputBuf = this->fHistory[this->fHistory.size() - 1 - this->fCurHistIndex];
        ++this->fCurHistIndex;
        this->fInputCurrentChar = this->fInputBuf.length();
    } else if (e->matches(QKeySequence::MoveToNextLine)) {
        // If we're at the latest command, don't do anything.
        if (this->fCurHistIndex == 0) {
            return;
        }
        --this->fCurHistIndex;
        // If the next command is the latest one, it means it's current
        // new command we backed up previously. So restore it. If not,
        // recall the next command from the history.
        if (this->fCurHistIndex == 0) {
            this->fInputBuf = this->fInputBufBackup;
            this->fInputBufBackup.clear();
        } else {
            this->fInputBuf = this->fHistory[this->fHistory.size() - this->fCurHistIndex];
            this->fInputCurrentChar = this->fInputBuf.length();
        }
        this->fInputCurrentChar = this->fInputBuf.length();
    } else if (e->matches(QKeySequence::SelectPreviousChar)) {
        //this->fTadsBuffer->move_left(true, false);
    } else if (e->matches(QKeySequence::SelectNextChar)) {
        //this->fTadsBuffer->move_right(true, false);
    } else if (e->matches(QKeySequence::SelectPreviousWord)) {
        //this->fTadsBuffer->move_left(true, true);
    } else if (e->matches(QKeySequence::SelectNextWord)) {
        //this->fTadsBuffer->move_right(true, true);
    } else if (e->matches(QKeySequence::SelectStartOfLine) or e->matches(QKeySequence::SelectStartOfBlock)) {
        //this->fTadsBuffer->start_of_line(true);
    } else if (e->matches(QKeySequence::SelectEndOfLine) or e->matches(QKeySequence::SelectEndOfBlock)) {
        //this->fTadsBuffer->end_of_line(true);
    } else if (e->matches(QKeySequence::SelectAll)) {
        //this->fTadsBuffer->start_of_line(false);
        //this->fTadsBuffer->end_of_line(true);
    } else if (e->matches(QKeySequence::Undo)) {
        //this->fTadsBuffer->undo();
    } else if (e->matches(QKeySequence::Copy)) {
        return;
    } else if (e->key() == Qt::Key_Backspace) {
        if (this->fInputCurrentChar > 0 and not this->fInputBuf.isEmpty()) {
            //this->fCursorPos.setX(this->fCursorPos.x() - metr.width(this->fInputBuf.at(this->fInputCurrentChar - 1)));
            this->fInputBuf.remove(this->fInputCurrentChar - 1, 1);
            --this->fInputCurrentChar;
        }
    } else {
        QString strToAdd = e->text();
        if (e->matches(QKeySequence::Paste)) {
            strToAdd = QApplication::clipboard()->text();
        } else if (strToAdd.isEmpty() or not strToAdd.at(0).isPrint()) {
            QWidget::keyPressEvent(e);
            return;
        }
        this->fInputBuf.insert(this->fInputCurrentChar, strToAdd);
        this->fInputCurrentChar += strToAdd.length();
    }
    this->updateCursorPos();
    this->update();
}


void
HFrame::inputMethodEvent( QInputMethodEvent* e )
{
    if (not hApp->gameRunning() or (this->fInputMode == NormalInput and e->commitString().isEmpty())) {
        QWidget::inputMethodEvent(e);
        return;
    }

    // If the keypress doesn't correspond to exactly one character, ignore
    // it.
    if (e->commitString().size() != 1) {
        QWidget::inputMethodEvent(e);
        return;
    }
    this->fKeyQueue.enqueue(e->commitString().at(0).toAscii());
}


void
HFrame::singleKeyPressEvent( QKeyEvent* event )
{
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(this->fInputMode == NoInput);

    switch (event->key()) {
      case 0:
      case Qt::Key_unknown:
        QWidget::keyPressEvent(event);
        return;

      case Qt::Key_Left:
        this->fKeyQueue.enqueue(8);
        break;

      case Qt::Key_Up:
        this->fKeyQueue.enqueue(11);
        break;

      case Qt::Key_Right:
        this->fKeyQueue.enqueue(21);
        break;

      case Qt::Key_Down:
        this->fKeyQueue.enqueue(10);
        break;

      default:
        // If the keypress doesn't correspond to exactly one character, ignore
        // it.
        if (event->text().size() != 1) {
            QWidget::keyPressEvent(event);
            return;
        }
        this->fKeyQueue.enqueue(event->text().at(0).toAscii());
    }
}


void
HFrame::mousePressEvent( QMouseEvent* e )
{
    if (this->fInputMode == NoInput) {
        this->fKeyQueue.append(0);
        this->fClickQueue.append(e->pos());
    }
    e->accept();
}


void
HFrame::getInput( char* buf, size_t buflen, int xPos, int yPos )
{
    this->flushText();
    //qDebug() << Q_FUNC_INFO;
    Q_ASSERT(buf != 0);

    this->fInputReady = false;
    this->fInputMode = NormalInput;
    this->fInputStartX = xPos;
    this->fInputStartY = yPos;
    this->fInputCurrentChar = 0;

    // Wait for a complete input line.
    while (hApp->gameRunning() and not this->fInputReady) {
        hApp->advanceEventLoop(QEventLoop::WaitForMoreEvents | QEventLoop::AllEvents);
    }

    // Make the input text part of the display pixmap.
    this->printText(this->fInputBuf.toAscii().constData(), this->fInputStartX, this->fInputStartY);

    qstrncpy(buf, this->fInputBuf.toAscii(), buflen);
    //qDebug() << this->fInputBuf;
    this->fInputBuf.clear();
}


int
HFrame::getNextKey()
{
    this->flushText();
    //qDebug() << Q_FUNC_INFO;

    //this->scrollDown();

    // If we have a key waiting, return it.
    if (not this->fKeyQueue.isEmpty()) {
        return this->fKeyQueue.dequeue();
    }

    // Wait for at least a key to become available.
    this->update();
    while (this->fKeyQueue.isEmpty() and hApp->gameRunning()) {
        hApp->advanceEventLoop(QEventLoop::WaitForMoreEvents | QEventLoop::AllEvents);
    }

    if (not hApp->gameRunning()) {
        // Game is quitting.
        return -3;
    }
    return this->fKeyQueue.dequeue();
}


QPoint
HFrame::getNextClick()
{
    Q_ASSERT(not this->fClickQueue.isEmpty());
    return this->fClickQueue.dequeue();
}


void
HFrame::clearRegion( int left, int top, int right, int bottom )
{
    this->flushText();
    if (left == 0 and top == 0 and right == 0 and bottom == 0) {
        this->fPixmap.fill(hugoColorToQt(this->fBgColor));
        return;
    }
    QPainter p(&this->fPixmap);
    p.fillRect(left, top, right - left + 1, bottom - top + 1, hugoColorToQt(this->fBgColor));
}


void
HFrame::setFontType( int hugoFont )
{
    this->flushText();
    this->fUseFixedFont = not (hugoFont & PROP_FONT);
    this->fUseUnderlineFont = hugoFont & UNDERLINE_FONT;
    this->fUseItalicFont = hugoFont & ITALIC_FONT;
    this->fUseBoldFont = hugoFont & BOLD_FONT;

    QFont f(this->fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
    f.setUnderline(this->fUseUnderlineFont);
    f.setItalic(this->fUseItalicFont);
    f.setBold(this->fUseBoldFont);
    this->fFontMetrics = QFontMetrics(f, &this->fPixmap);
}


void
HFrame::printText( const char* str, int x, int y )
{
    if (this->fPrintBuffer.isEmpty()) {
        this->fFlushXPos = x;
        this->fFlushYPos = y;
    }
    this->fPrintBuffer += QString::fromAscii(str);
}


void
HFrame::printImage( const QImage& img, int x, int y )
{
    this->flushText();
    QPainter p(&this->fPixmap);
    p.drawImage(x, y, img);
}


void
HFrame::scrollUp( int left, int top, int right, int bottom, int h )
{
    if (h == 0) {
        return;
    }

    this->flushText();
    QRegion exposed;
    ++right;
    ++bottom;

#if QT_VERSION < 0x040600
    // Qt versions prior to 4.6 lack the QPixmap::scroll() routine. For
    // those versions we implement a (slower) fallback, where we simply
    // paint the contents from source to destination.
    QRect scrRect(left, top, right - left, bottom - top);
    QRect dest = scrRect & fPixmap.rect();
    QRect src = dest.translated(0, h) & dest;
    if (src.isEmpty()) {
        return;
    }

    //this->fPixmap.detach();
    QPixmap pix = this->fPixmap;
    QPainter p(&pix);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(src.translated(0, -h), this->fPixmap, src);
    p.end();
    this->fPixmap = pix;

    exposed += dest;
    exposed -= src.translated(0, -h);
#else
    // Qt 4.6 and newer have scroll(), which is fast and nice.
    this->fPixmap.scroll(0, -h, left, top, right - left, bottom - top, &exposed);
#endif

    // Fill exposed region.
    const QRect& r = exposed.boundingRect();
    this->clearRegion(r.left(), r.top(), r.left() + r.width(), r.top() + r.bottom());
    if (hApp->settings()->softTextScrolling) {
        hApp->advanceEventLoop();
    }
}


void
HFrame::flushText()
{
    if (this->fPrintBuffer.isEmpty())
        return;

    QFont f(this->fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
    f.setUnderline(this->fUseUnderlineFont);
    f.setItalic(this->fUseItalicFont);
    f.setBold(this->fUseBoldFont);
    QFontMetrics m(f);
    QPainter p(&this->fPixmap);
    p.setFont(f);
    p.setPen(hugoColorToQt(this->fFgColor));
    p.setBackgroundMode(Qt::OpaqueMode);
    p.setBackground(QBrush(hugoColorToQt(this->fBgColor)));
    /*
    for (int i = 0; i < this->fPrintBuffer.length(); ++i) {
        p.drawText(this->fFlushXPos, this->fFlushYPos + m.ascent(), QString(this->fPrintBuffer.at(i)));
        this->fFlushXPos += m.width(this->fPrintBuffer.at(i));
    }
    */
    p.drawText(this->fFlushXPos, this->fFlushYPos + m.ascent(), this->fPrintBuffer);
    //qDebug() << this->fPrintBuffer;
    this->fPrintBuffer.clear();
}


void
HFrame::updateCursorPos()
{
    // Reset the blink timer.
    if (this->fBlinkTimer->isActive()) {
        this->fBlinkTimer->start();
    }

    // Blink-out first to ensure the cursor won't stay visible at the previous
    // position after we move it.
    if (this->fBlinkVisible) {
        this->fBlinkCursor();
    }

    // Blink-in.
    if (not this->fBlinkVisible) {
        this->fBlinkCursor();
    }

    int xOffs = this->currentFontMetrics().width(this->fInputBuf.left(this->fInputCurrentChar));
    this->moveCursorPos(QPoint(this->fInputStartX + xOffs, this->fInputStartY));

    // Blink-in.
    if (not this->fBlinkVisible) {
        this->fBlinkCursor();
    }
}


void
HFrame::resetCursorBlinking()
{
    // Start the timer unless cursor blinking is disabled.
    if (QApplication::cursorFlashTime() > 1) {
        this->fBlinkTimer->start(QApplication::cursorFlashTime() / 2);
    }
}
