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
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QClipboard>
#include <QPainter>
#include <QTimer>
#include <QTextCodec>
#include <QMenu>

extern "C" {
#include "heheader.h"
}
#include "hframe.h"
#include "happlication.h"
#include "hmainwindow.h"
#include "hmarginwidget.h"
#include "hugodefs.h"
#include "settings.h"
#include "hugohandlers.h"


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
      fFgColor(16), // Default text color
      fBgColor(17), // Default background color
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
      fBlinkTimer(new QTimer(this)),
      fNeedScreenUpdate(false)
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

    // Requesting scrollback simply triggers the scrollback window.
    // Since focus is lost, subsequent scrolling/paging events will work as expected.
    connect(this, SIGNAL(requestScrollback()), hMainWin, SLOT(showScrollback()));

    this->setAttribute(Qt::WA_InputMethodEnabled);
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    hFrame = this;
}


void
HFrame::fEnqueueKey(char key, QMouseEvent* e)
{
    QMutexLocker mKeyLocker(&fKeyQueueMutex);
    QMutexLocker mClickLocker(&fClickQueueMutex);
    // Only allow one keypress or click in the queue. No reason to use a queue
    // for that, but we might still want the possibilty to change this to allow
    // multiple keys/clicks to queue up, if the need for that arises.
    if (fKeyQueue.empty()) {
        fKeyQueue.enqueue(key);
        if (e) {
            fClickQueue.append(e->pos());
        }
    }
    mKeyLocker.unlock();
    mClickLocker.unlock();
    keypressAvailableWaitCond.wakeAll();
}


void
HFrame::fBlinkCursor()
{
    this->fBlinkVisible = not this->fBlinkVisible;
    this->update(this->fCursorPos.x(), this->fCursorPos.y() + 1, this->fCursorPos.x() + 2,
                 this->fCursorPos.y() + this->fHeight + 2);
}


void
HFrame::fHandleFocusChange( QWidget* old, QWidget* now )
{
    if (now == 0) {
        // The application window lost focus.  Disable cursor blinking.
        this->fBlinkTimer->stop();
#ifdef Q_OS_MAC
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
        // Minimize the application if we're fullscreen (except on OSX.)
        if (hMainWin->isFullScreen()) {
#ifndef Q_OS_MAC
            hMainWin->showMinimized();
#endif
            if (hApp->settings()->muteWhenMinimized) {
                muteSound(true);
                muteVideo(true);
            }
        }
    } else if (old == 0 and now != 0) {
        // The application window gained focus.  Reset cursor blinking and
        // unmute (just in case we were muted previously.)
        this->resetCursorBlinking();
        muteSound(false);
        muteVideo(false);
    }
}


void
HFrame::fEndInputMode( bool addToHistory )
{
    this->fInputReady = true;
    this->fInputMode = NoInput;
    // The current command only needs to be appended to the history if
    // it's not empty and differs from the previous command in the history.
    if (((this->fHistory.isEmpty() and not fInputBuf.isEmpty())
         or (not this->fHistory.isEmpty()
             and not fInputBuf.isEmpty()
             and fInputBuf != this->fHistory.last()))
        and addToHistory)
    {
        this->fHistory.append(fInputBuf);
        // If we're about to overflow the max history cap, delete the
        // oldest command from the history.
        if (this->fHistory.size() > this->fMaxHistCap) {
            this->fHistory.removeFirst();
        }
    }
    this->fCurHistIndex = 0;
    // Make the input text part of the display pixmap.
    printText(fInputBuf.toLatin1().constData(), fInputStartX, fInputStartY);
    inputLineWaitCond.wakeAll();
}


void
HFrame::paintEvent( QPaintEvent* e )
{
    //qDebug(Q_FUNC_INFO);
    QPainter p(this);
    p.setClipRegion(e->region());
    p.drawPixmap(e->rect(), fPixmap, e->rect());

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
        p.drawText(this->fInputStartX, this->fInputStartY + m.ascent() + 1, this->fInputBuf);
    }

    // Likewise, the input caret needs to be painted on top of the input text.
    if (this->fCursorVisible and this->fBlinkVisible) {
        p.setPen(hugoColorToQt(this->fFgColor));
        p.drawLine(this->fCursorPos.x(), this->fCursorPos.y() + 1,
                   this->fCursorPos.x(), this->fCursorPos.y() + 1 + this->fHeight);
    }
}


void
HFrame::resizeEvent( QResizeEvent* e )
{
    // Ignore invalid resizes.  No idea why that happens sometimes, but it
    // does (we get negative values for width or height.)
    if (not e->size().isValid()) {
        e->ignore();
        return;
    }

    // Save a copy of the current pixmaps.
    const QPixmap& tmp = fPixmap.copy();

    // Adjust the margins so that we get our final size.
    hApp->updateMargins(-1);

    // Create new pixmaps, using the new size and fill it with the default
    // background color.
    QPixmap newPixmap(size());
    newPixmap.fill(hugoColorToQt(fBgColor));

    // Draw the saved pixmaps into the new ones and use them as our new
    // display.
    QPainter p(&newPixmap);
    p.drawPixmap(0, 0, tmp);
    fPixmap = newPixmap;

    hHandlers->settextmode();
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

    if (e->key() == Qt::Key_Escape) {
        emit escKeyPressed();
    }

    if (this->fInputMode == NoInput) {
        this->singleKeyPressEvent(e);
        return;
    }

    // Just for having shorter identifiers.
    int& i = this->fInputCurrentChar;
    QString& buf = this->fInputBuf;

    // Enable mouse tracking when hiding the cursor so that we can
    // restore it when the mouse is moved.
    hApp->marginWidget()->setCursor(Qt::BlankCursor);
    this->setMouseTracking(true);
    hApp->marginWidget()->setMouseTracking(true);

    if (e->matches(QKeySequence::MoveToStartOfLine) or e->matches(QKeySequence::MoveToStartOfBlock)) {
        i = 0;
    } else if (e->matches(QKeySequence::MoveToEndOfLine) or e->matches(QKeySequence::MoveToEndOfBlock)) {
        i = buf.length();
    } else if (e->matches(QKeySequence::InsertParagraphSeparator)) {
        fEndInputMode(true);
        return;
    } else if (e->matches(QKeySequence::Delete)) {
        if (i < buf.length()) {
            buf.remove(i, 1);
        }
    } else if (e->matches(QKeySequence::DeleteEndOfWord)) {
        // Delete all non-alphanumerics first.
        while (i < buf.length() and not buf.at(i).isLetterOrNumber()) {
            buf.remove(i, 1);
        }
        // Delete all alphanumerics.
        while (i < buf.length() and buf.at(i).isLetterOrNumber()) {
            buf.remove(i, 1);
        }
    } else if (e->matches(QKeySequence::DeleteStartOfWord)) {
        // Delete all non-alphanumerics first.
        while (i > 0 and not buf.at(i - 1).isLetterOrNumber()) {
            buf.remove(i - 1, 1);
            --i;
        }
        // Delete all alphanumerics.
        while (i > 0 and buf.at(i - 1).isLetterOrNumber()) {
            buf.remove(i - 1, 1);
            --i;
        }
    } else if (e->matches(QKeySequence::MoveToPreviousChar)) {
        if (i > 0)
            --i;
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        if (i < buf.length())
            ++i;
    } else if (e->matches(QKeySequence::MoveToPreviousWord)) {
        // Skip all non-alphanumerics first.
        while (i > 0 and not buf.at(i - 1).isLetterOrNumber()) {
            --i;
        }
        // Skip all alphanumerics.
        while (i > 0 and buf.at(i - 1).isLetterOrNumber()) {
            --i;
        }
    } else if (e->matches(QKeySequence::MoveToNextWord)) {
        // Skip all non-alphanumerics first.
        while (i < buf.length() and not buf.at(i).isLetterOrNumber()) {
            ++i;
        }
        // Skip all alphanumerics.
        while (i < buf.length() and buf.at(i).isLetterOrNumber()) {
            ++i;
        }
    } else if (e->matches(QKeySequence::MoveToPreviousLine)) {
        // If we're already at the oldest command in the history, or
        // the history list is empty, don't do anything.
        if (this->fCurHistIndex == this->fHistory.size() or this->fHistory.isEmpty()) {
            return;
        }
        // If the current command is new and not in the history yet,
        // remember it so we can bring it back if the user recalls it.
        if (this->fCurHistIndex == 0) {
            this->fInputBufBackup = buf;
        }
        // Recall the previous command from the history.
        buf = this->fHistory[this->fHistory.size() - 1 - this->fCurHistIndex];
        ++this->fCurHistIndex;
        i = buf.length();
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
            buf = this->fInputBufBackup;
            this->fInputBufBackup.clear();
        } else {
            buf = this->fHistory[this->fHistory.size() - this->fCurHistIndex];
            i = buf.length();
        }
        i = buf.length();
    } else if (e->matches(QKeySequence::MoveToPreviousPage)) {
        this->requestScrollback();
    } else if (e->key() == Qt::Key_Backspace) {
        if (i > 0 and not buf.isEmpty()) {
            buf.remove(i - 1, 1);
            --i;
        }
    } else {
        QString strToAdd = e->text();
        if (e->matches(QKeySequence::Paste)) {
            strToAdd = QApplication::clipboard()->text();
        } else if (strToAdd.isEmpty() or not strToAdd.at(0).isPrint()) {
            QWidget::keyPressEvent(e);
            return;
        }
        buf.insert(i, strToAdd);
        i += strToAdd.length();
    }
    this->updateCursorPos();
}


void
HFrame::inputMethodEvent( QInputMethodEvent* e )
{
    if (not hApp->gameRunning() or (this->fInputMode == NormalInput and e->commitString().isEmpty())) {
        QWidget::inputMethodEvent(e);
        return;
    }

    // Enable mouse tracking when hiding the cursor so that we can
    // restore it when the mouse is moved.
    hApp->marginWidget()->setCursor(Qt::BlankCursor);
    this->setMouseTracking(true);
    hApp->marginWidget()->setMouseTracking(true);

    const QByteArray& bytes = hApp->hugoCodec()->fromUnicode(e->commitString());
    // If the keypress doesn't correspond to exactly one character, ignore
    // it.
    if (bytes.size() != 1) {
        QWidget::inputMethodEvent(e);
        return;
    }
    fEnqueueKey(bytes[0], 0);
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
        fEnqueueKey(8, 0);
        break;

      case Qt::Key_Up:
        fEnqueueKey(11, 0);
        break;

      case Qt::Key_Right:
        fEnqueueKey(21, 0);
        break;

      case Qt::Key_Down:
        fEnqueueKey(10, 0);
        break;

      default:
        // If the keypress doesn't correspond to exactly one character, ignore
        // it.
        if (event->text().size() != 1) {
            QWidget::keyPressEvent(event);
            return;
        }
        fEnqueueKey(event->text().at(0).toLatin1(), 0);
    }
}


void
HFrame::mousePressEvent( QMouseEvent* e )
{
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    if (this->fInputMode == NoInput) {
        fEnqueueKey(0, e);
    }
    e->accept();
}


void
HFrame::mouseDoubleClickEvent( QMouseEvent* e )
{
    if (this->fInputMode != NormalInput or e->button() != Qt::LeftButton) {
        return;
    }
    // Get the word at the double click position.
    QString word(TB_FindWord(e->x(), e->y()));
    if (word.isEmpty()) {
        // No word found.
        return;
    }
    this->insertInputText(word, false, false);
}


void
HFrame::mouseMoveEvent( QMouseEvent* e )
{
    if (this->cursor().shape() == Qt::BlankCursor) {
        this->setMouseTracking(false);
    }
    // Pass it on to our parent.
    e->ignore();
}


void
HFrame::startInput( int xPos, int yPos )
{
    //qDebug() << Q_FUNC_INFO;
    updateGameScreen(false);
    this->fInputReady = false;
    this->fInputMode = NormalInput;
    this->fInputStartX = xPos;
    this->fInputStartY = yPos;
    this->fInputCurrentChar = 0;

    QMutexLocker mKeyLocker(&fKeyQueueMutex);
    QMutexLocker mClickLocker(&fClickQueueMutex);
    fKeyQueue.clear();
    fClickQueue.clear();
}


void
HFrame::getInput(char* buf, size_t buflen)
{
    Q_ASSERT(buf != 0);
    qstrncpy(buf, this->fInputBuf.toLatin1(), buflen);
    fInputBuf.clear();
}


int
HFrame::getNextKey()
{
    //qDebug() << Q_FUNC_INFO;
    QMutexLocker mLocker(&fKeyQueueMutex);
    Q_ASSERT(not fKeyQueue.isEmpty());
    return fKeyQueue.dequeue();
}


QPoint
HFrame::getNextClick()
{
    QMutexLocker mLocker(&fClickQueueMutex);
    Q_ASSERT(not this->fClickQueue.isEmpty());
    return this->fClickQueue.dequeue();
}


bool
HFrame::hasKeyInQueue()
{
    QMutexLocker mLocker(&fKeyQueueMutex);
    return not fKeyQueue.isEmpty();
}


void
HFrame::clearRegion( int left, int top, int right, int bottom )
{
    //qDebug(Q_FUNC_INFO);
    flushText();
    fNeedScreenUpdate = true;
    if (left == 0 and top == 0 and right == 0 and bottom == 0) {
        fPixmap.fill(hugoColorToQt(this->fBgColor));
        return;
    }
    QPainter p(&fPixmap);
    QRect rect(left, top, right - left + 1, bottom - top + 1);
    p.fillRect(rect, hugoColorToQt(this->fBgColor));

    // If this was a fullscreen clear, then also clear the margin color.
    if (rect == fPixmap.rect()) {
        hApp->updateMargins(fBgColor);
    }
}


void
HFrame::setFgColor(int color)
{
    this->flushText();
    this->fFgColor = color;
}


void
HFrame::setBgColor(int color)
{
    this->flushText();
    this->fBgColor = color;
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
    this->fFontMetrics = QFontMetrics(f, &fPixmap);

    // Adjust text caret for new font.
    this->setCursorHeight(this->fFontMetrics.height());
}


void
HFrame::printText( const QString& str, int x, int y )
{
    if (this->fPrintBuffer.isEmpty()) {
        this->fFlushXPos = x;
        this->fFlushYPos = y;
    }
    this->fPrintBuffer += str;
}


void
HFrame::printImage( const QImage& img, int x, int y )
{
    this->flushText();
    QPainter p(&fPixmap);
    p.drawImage(x, y, img);
    fNeedScreenUpdate = true;
}


void
HFrame::scrollUp( int left, int top, int right, int bottom, int h )
{
    if (h == 0) {
        return;
    }

    flushText();
    QRegion exposed;
    ++right;
    ++bottom;
    fPixmap.scroll(0, -h, left, top, right - left, bottom - top, &exposed);
    fNeedScreenUpdate = true;

    // Fill exposed region.
    const QRect& r = exposed.boundingRect();
    this->clearRegion(r.left(), r.top(), r.left() + r.width(), r.top() + r.bottom());

    if (hApp->settings()->softTextScrolling) {
        QEventLoop idleLoop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, SIGNAL(timeout()), &idleLoop, SLOT(quit()));
        QObject::connect(hApp, SIGNAL(gameQuitting()), &idleLoop, SLOT(quit()));
        timer.start(12);
        idleLoop.exec();
    }
    updateGameScreen(false);
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
    QPainter p(&fPixmap);
    p.setFont(f);
    p.setPen(hugoColorToQt(this->fFgColor));
    p.setBackgroundMode(Qt::OpaqueMode);
    p.setBackground(QBrush(hugoColorToQt(this->fBgColor)));
    p.drawText(this->fFlushXPos, this->fFlushYPos + currentFontMetrics().ascent() + 1,
               this->fPrintBuffer);
    this->fPrintBuffer.clear();
    fNeedScreenUpdate = true;
}


void
HFrame::updateGameScreen(bool force)
{
    flushText();
    if (fNeedScreenUpdate or force) {
        //qDebug(Q_FUNC_INFO);
        hApp->updateMargins(this->fBgColor);
        fNeedScreenUpdate = false;
        hApp->marginWidget()->update();
        update();
    }
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
    update();
}


void
HFrame::resetCursorBlinking()
{
    // Start the timer unless cursor blinking is disabled.
    if (QApplication::cursorFlashTime() > 1) {
        this->fBlinkTimer->start(QApplication::cursorFlashTime() / 2);
    }
}


void
HFrame::insertInputText( QString txt, bool execute, bool clearCurrent )
{
    if (this->fInputMode != NormalInput) {
        return;
    }
    // Clear the current input, if requested.
    if (clearCurrent) {
        this->fInputBuf.clear();
        this->fInputCurrentChar = 0;
    }
    // If the command is not to be executed, append a space so it won't run
    // together with what the user types next.
    if (not execute) {
        txt.append(' ');
    }
    this->fInputBuf.insert(this->fInputCurrentChar, txt);
    this->fInputCurrentChar += txt.length();
    this->updateCursorPos();
    if (execute) {
        fEndInputMode(false);
    }
}


QList<const QAction*>
HFrame::getGameContextMenuEntries( QMenu& dst )
{
    QList<const QAction*> actions;
    if (fInputMode != NormalInput) {
        return actions;
    }
    for (int i = 0; i < context_commands; ++i) {
        if (qstrcmp(context_command[i], "-") == 0) {
            dst.addSeparator();
        } else {
            actions.append(dst.addAction(context_command[i]));
        }
    }
    return actions;
}
