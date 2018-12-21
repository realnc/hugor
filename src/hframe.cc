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
#include "hframe.h"

#include <QClipboard>
#include <QDebug>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QTextCodec>
#include <QTimer>

#include "happlication.h"
extern "C" {
#include "heheader.h"
}
#include "hmainwindow.h"
#include "hmarginwidget.h"
#include "hugodefs.h"
#include "hugohandlers.h"
#include "settings.h"

HFrame* hFrame = nullptr;

HFrame::HFrame(QWidget* parent)
    : QWidget(parent)
    , fHeight(QFontMetrics(hApp->settings()->propFont).height())
    , fBlinkTimer(new QTimer(this))
    , fMinimizeTimer(new QTimer(this))
{
    // We handle player input, so we need to accept focus.
    setFocusPolicy(Qt::WheelFocus);

    connect(fBlinkTimer, SIGNAL(timeout()), this, SLOT(fBlinkCursor()));
    resetCursorBlinking();
    // setCursorVisible(true);

    // We need to check whether the application lost focus.
    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)),
            SLOT(fHandleFocusChange(QWidget*, QWidget*)));

    fMinimizeTimer->setSingleShot(true);
    connect(fMinimizeTimer, SIGNAL(timeout()), SLOT(fHandleFocusLost()));

    // Requesting scrollback simply triggers the scrollback window. Since focus is lost, subsequent
    // scrolling/paging events will work as expected.
    connect(this, SIGNAL(requestScrollback()), hMainWin, SLOT(showScrollback()));

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_OpaquePaintEvent);
    hFrame = this;
}

void HFrame::fEnqueueKey(char key, QMouseEvent* e)
{
    QMutexLocker mKeyLocker(&fKeyQueueMutex);
    QMutexLocker mClickLocker(&fClickQueueMutex);
    // Only allow one keypress or click in the queue. No reason to use a queue for that, but we
    // might still want the possibilty to change this to allow multiple keys/clicks to queue up, if
    // the need for that arises.
    if (fKeyQueue.empty()) {
        fKeyQueue.enqueue(key);
        if (e != nullptr) {
            fClickQueue.append(e->pos());
        }
    }
    mKeyLocker.unlock();
    mClickLocker.unlock();
    keypressAvailableWaitCond.wakeAll();
}

void HFrame::fBlinkCursor()
{
    fBlinkVisible = not fBlinkVisible;
    update(fCursorPos.x(), fCursorPos.y() + 1, fCursorPos.x() + 2, fCursorPos.y() + fHeight + 2);
}

void HFrame::fHandleFocusChange(QWidget* old, QWidget* now)
{
    if (now == nullptr) {
        // Minimize a bit later, in case we only lose focus for a very short time.
        fMinimizeTimer->start(40);
    } else if (old == nullptr and now != nullptr) {
        // In case we only lost focus only for a short time, abort any pending minimize operation.
        fMinimizeTimer->stop();

        // The application window gained focus.  Reset cursor blinking and unmute (just in case we
        // were muted previously.)
        resetCursorBlinking();
        muteSound(false);
        muteVideo(false);
    }
}

void HFrame::fHandleFocusLost()
{
    // The application window lost focus.  Disable cursor blinking.
    fBlinkTimer->stop();
#ifdef Q_OS_MAC
    // On the Mac, when applications lose focus the cursor must be disabled.
    if (fBlinkVisible) {
        fBlinkCursor();
    }
#else
    // On all other systems we assume the cursor must stay visible.
    if (not fBlinkVisible) {
        fBlinkCursor();
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
}

void HFrame::fEndInputMode(bool addToHistory)
{
    fInputReady = true;
    fInputMode = InputMode::None;
    // The current command only needs to be appended to the history if it's not empty and differs
    // from the previous command in the history.
    if (((fHistory.isEmpty() and not fInputBuf.isEmpty())
         or (not fHistory.isEmpty() and not fInputBuf.isEmpty() and fInputBuf != fHistory.last()))
        and addToHistory) {
        fHistory.append(fInputBuf);
        // If we're about to overflow the max history cap, delete the oldest command from the
        // history.
        if (fHistory.size() > fMaxHistCap) {
            fHistory.removeFirst();
        }
    }
    fCurHistIndex = 0;
    // Make the input text part of the display pixmap.
    printText(fInputBuf.toLatin1().constData(), fInputStartX, fInputStartY);
    inputLineWaitCond.wakeAll();
}

void HFrame::paintEvent(QPaintEvent* e)
{
    // qDebug(Q_FUNC_INFO);
    QPainter p(this);
    p.setClipRegion(e->region());
    p.drawPixmap(e->rect(), fPixmap, e->rect());

    // Draw our current input. We need to do this here, after the pixmap has already been painted,
    // so that the input gets painted on top. Otherwise, we could not erase text during editing.
    if (fInputMode == InputMode::Normal and not fInputBuf.isEmpty()) {
        QFont f(fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
        f.setUnderline(fUseUnderlineFont);
        f.setItalic(fUseItalicFont);
        f.setBold(fUseBoldFont);
        QFontMetrics m(f);
        p.setFont(f);
        p.setPen(hugoColorToQt(fFgColor));
        p.setBackgroundMode(Qt::OpaqueMode);
        p.setBackground(QBrush(hugoColorToQt(fBgColor)));
        p.drawText(fInputStartX, fInputStartY + m.ascent() + 1, fInputBuf);
    }

    // Likewise, the input caret needs to be painted on top of the input text.
    if (fCursorVisible and fBlinkVisible) {
        p.setPen(hugoColorToQt(fFgColor));
        p.drawLine(fCursorPos.x(), fCursorPos.y() + 1, fCursorPos.x(),
                   fCursorPos.y() + 1 + fHeight);
    }
}

void HFrame::resizeEvent(QResizeEvent* e)
{
    // Ignore invalid resizes. No idea why that happens sometimes, but it does (we get negative
    // values for width or height.)
    if (not e->size().isValid()) {
        e->ignore();
        return;
    }

    // Save a copy of the current pixmaps.
    const QPixmap& tmp = fPixmap.copy();

    // Adjust the margins so that we get our final size.
    hApp->updateMargins(-1);

    // Create new pixmaps, using the new size and fill it with the default background color.
    QPixmap newPixmap(size());
    newPixmap.fill(hugoColorToQt(fBgColor));

    // Draw the saved pixmaps into the new ones and use them as our new display.
    QPainter p(&newPixmap);
    p.drawPixmap(0, 0, tmp);
    fPixmap = newPixmap;

    hHandlers->settextmode();
    display_needs_repaint = true;
}

void HFrame::keyPressEvent(QKeyEvent* e)
{
    // qDebug() << Q_FUNC_INFO;

    // qDebug() << "Key pressed:" << hex << e->key();

    if (not hApp->gameRunning()) {
        QWidget::keyPressEvent(e);
        return;
    }

    if (e->key() == Qt::Key_Escape) {
        emit escKeyPressed();
    }

    if (fInputMode == InputMode::None) {
        singleKeyPressEvent(e);
        return;
    }

    // Just for having shorter identifiers.
    int& i = fInputCurrentChar;
    QString& buf = fInputBuf;

    // Enable mouse tracking when hiding the cursor so that we can restore it when the mouse is
    // moved.
    hApp->marginWidget()->setCursor(Qt::BlankCursor);
    setMouseTracking(true);
    hApp->marginWidget()->setMouseTracking(true);

    if (e->matches(QKeySequence::MoveToStartOfLine)
        or e->matches(QKeySequence::MoveToStartOfBlock)) {
        i = 0;
    } else if (e->matches(QKeySequence::MoveToEndOfLine)
               or e->matches(QKeySequence::MoveToEndOfBlock)) {
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
        if (i > 0) {
            --i;
        }
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        if (i < buf.length()) {
            ++i;
        }
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
        // If we're already at the oldest command in the history, or the history list is empty,
        // don't do anything.
        if (fCurHistIndex == fHistory.size() or fHistory.isEmpty()) {
            return;
        }
        // If the current command is new and not in the history yet, remember it so we can bring it
        // back if the user recalls it.
        if (fCurHistIndex == 0) {
            fInputBufBackup = buf;
        }
        // Recall the previous command from the history.
        buf = fHistory[fHistory.size() - 1 - fCurHistIndex];
        ++fCurHistIndex;
        i = buf.length();
    } else if (e->matches(QKeySequence::MoveToNextLine)) {
        // If we're at the latest command, don't do anything.
        if (fCurHistIndex == 0) {
            return;
        }
        --fCurHistIndex;
        // If the next command is the latest one, it means it's current new command we backed up
        // previously. So restore it. If not, recall the next command from the history.
        if (fCurHistIndex == 0) {
            buf = fInputBufBackup;
            fInputBufBackup.clear();
        } else {
            buf = fHistory[fHistory.size() - fCurHistIndex];
            i = buf.length();
        }
        i = buf.length();
    } else if (e->matches(QKeySequence::MoveToPreviousPage)) {
        emit requestScrollback();
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
    updateCursorPos();
}

void HFrame::inputMethodEvent(QInputMethodEvent* e)
{
    if (not hApp->gameRunning()
        or (fInputMode == InputMode::Normal and e->commitString().isEmpty())) {
        QWidget::inputMethodEvent(e);
        return;
    }

    // Enable mouse tracking when hiding the cursor so that we can restore it when the mouse is
    // moved.
    hApp->marginWidget()->setCursor(Qt::BlankCursor);
    setMouseTracking(true);
    hApp->marginWidget()->setMouseTracking(true);

    const QByteArray& bytes = hApp->hugoCodec()->fromUnicode(e->commitString());
    // If the keypress doesn't correspond to exactly one character, ignore it.
    if (bytes.size() != 1) {
        QWidget::inputMethodEvent(e);
        return;
    }
    fEnqueueKey(bytes[0], nullptr);
}

void HFrame::singleKeyPressEvent(QKeyEvent* event)
{
    // qDebug() << Q_FUNC_INFO;
    Q_ASSERT(fInputMode == InputMode::None);

    switch (event->key()) {
    case 0:
    case Qt::Key_unknown:
        QWidget::keyPressEvent(event);
        return;

    case Qt::Key_Left:
        fEnqueueKey(8, nullptr);
        break;

    case Qt::Key_Up:
        fEnqueueKey(11, nullptr);
        break;

    case Qt::Key_Right:
        fEnqueueKey(21, nullptr);
        break;

    case Qt::Key_Down:
        fEnqueueKey(10, nullptr);
        break;

    default:
        // If the keypress doesn't correspond to exactly one character, ignore it.
        if (event->text().size() != 1) {
            QWidget::keyPressEvent(event);
            return;
        }
        fEnqueueKey(event->text().at(0).toLatin1(), nullptr);
    }
}

void HFrame::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    if (fInputMode == InputMode::None) {
        fEnqueueKey(0, e);
    }
    e->accept();
}

void HFrame::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (fInputMode != InputMode::Normal or e->button() != Qt::LeftButton) {
        return;
    }
    // Get the word at the double click position.
    QString word(TB_FindWord(e->x(), e->y()));
    if (word.isEmpty()) {
        // No word found.
        return;
    }
    insertInputText(word, false, false);
}

void HFrame::mouseMoveEvent(QMouseEvent* e)
{
    if (cursor().shape() == Qt::BlankCursor) {
        setMouseTracking(false);
    }
    // Pass it on to our parent.
    e->ignore();
}

void HFrame::startInput(int xPos, int yPos)
{
    // qDebug() << Q_FUNC_INFO;
    updateGameScreen(false);
    fInputReady = false;
    fInputMode = InputMode::Normal;
    fInputStartX = xPos;
    fInputStartY = yPos;
    fInputCurrentChar = 0;

    QMutexLocker mKeyLocker(&fKeyQueueMutex);
    QMutexLocker mClickLocker(&fClickQueueMutex);
    fKeyQueue.clear();
    fClickQueue.clear();
}

void HFrame::getInput(char* buf, size_t buflen)
{
    Q_ASSERT(buf != nullptr);
    qstrncpy(buf, fInputBuf.toLatin1(), buflen);
    fInputBuf.clear();
}

int HFrame::getNextKey()
{
    // qDebug() << Q_FUNC_INFO;
    QMutexLocker mLocker(&fKeyQueueMutex);
    Q_ASSERT(not fKeyQueue.isEmpty());
    return fKeyQueue.dequeue();
}

QPoint HFrame::getNextClick()
{
    QMutexLocker mLocker(&fClickQueueMutex);
    Q_ASSERT(not fClickQueue.isEmpty());
    return fClickQueue.dequeue();
}

bool HFrame::hasKeyInQueue()
{
    QMutexLocker mLocker(&fKeyQueueMutex);
    return not fKeyQueue.isEmpty();
}

void HFrame::clearRegion(int left, int top, int right, int bottom)
{
    // qDebug(Q_FUNC_INFO);
    flushText();
    fNeedScreenUpdate = true;
    if (left == 0 and top == 0 and right == 0 and bottom == 0) {
        fPixmap.fill(hugoColorToQt(fBgColor));
        return;
    }
    QPainter p(&fPixmap);
    QRect rect(left, top, right - left + 1, bottom - top + 1);
    p.fillRect(rect, hugoColorToQt(fBgColor));

    // If this was a fullscreen clear, then also clear the margin color.
    if (rect == fPixmap.rect()) {
        hApp->updateMargins(fBgColor);
    }
}

void HFrame::setFgColor(int color)
{
    flushText();
    fFgColor = color;
}

void HFrame::setBgColor(int color)
{
    flushText();
    fBgColor = color;
}

void HFrame::setFontType(int hugoFont)
{
    flushText();
    fUseFixedFont = not(hugoFont & PROP_FONT);
    fUseUnderlineFont = hugoFont & UNDERLINE_FONT;
    fUseItalicFont = hugoFont & ITALIC_FONT;
    fUseBoldFont = hugoFont & BOLD_FONT;

    QFont f(fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
    f.setUnderline(fUseUnderlineFont);
    f.setItalic(fUseItalicFont);
    f.setBold(fUseBoldFont);
    fFontMetrics = QFontMetrics(f, &fPixmap);

    // Adjust text caret for new font.
    setCursorHeight(fFontMetrics.height());
}

void HFrame::printText(const QString& str, int x, int y)
{
    if (fPrintBuffer.isEmpty()) {
        fFlushXPos = x;
        fFlushYPos = y;
    }
    fPrintBuffer += str;
}

void HFrame::printImage(const QImage& img, int x, int y)
{
    flushText();
    QPainter p(&fPixmap);
    p.drawImage(x, y, img);
    fNeedScreenUpdate = true;
}

void HFrame::scrollUp(int left, int top, int right, int bottom, int h)
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
    clearRegion(r.left(), r.top(), r.left() + r.width(), r.top() + r.bottom());

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

void HFrame::flushText()
{
    if (fPrintBuffer.isEmpty()) {
        return;
    }

    QFont f(fUseFixedFont ? hApp->settings()->fixedFont : hApp->settings()->propFont);
    f.setUnderline(fUseUnderlineFont);
    f.setItalic(fUseItalicFont);
    f.setBold(fUseBoldFont);
    QPainter p(&fPixmap);
    p.setFont(f);
    p.setPen(hugoColorToQt(fFgColor));
    p.setBackgroundMode(Qt::OpaqueMode);
    p.setBackground(QBrush(hugoColorToQt(fBgColor)));
    p.drawText(fFlushXPos, fFlushYPos + currentFontMetrics().ascent() + 1, fPrintBuffer);
    fPrintBuffer.clear();
    fNeedScreenUpdate = true;
}

void HFrame::updateGameScreen(bool force)
{
    flushText();
    if (fNeedScreenUpdate or force) {
        // qDebug(Q_FUNC_INFO);
        hApp->updateMargins(fBgColor);
        fNeedScreenUpdate = false;
        hApp->marginWidget()->update();
        update();
    }
}

void HFrame::updateCursorPos()
{
    // Reset the blink timer.
    if (fBlinkTimer->isActive()) {
        fBlinkTimer->start();
    }

    // Blink-out first to ensure the cursor won't stay visible at the previous position after we
    // move it.
    if (fBlinkVisible) {
        fBlinkCursor();
    }

    // Blink-in.
    if (not fBlinkVisible) {
        fBlinkCursor();
    }

    int xOffs = currentFontMetrics().width(fInputBuf.left(fInputCurrentChar));
    moveCursorPos(QPoint(fInputStartX + xOffs, fInputStartY));

    // Blink-in.
    if (not fBlinkVisible) {
        fBlinkCursor();
    }
    update();
}

void HFrame::resetCursorBlinking()
{
    // Start the timer unless cursor blinking is disabled.
    if (QApplication::cursorFlashTime() > 1) {
        fBlinkTimer->start(QApplication::cursorFlashTime() / 2);
    }
}

void HFrame::insertInputText(QString txt, bool execute, bool clearCurrent)
{
    if (fInputMode != InputMode::Normal) {
        return;
    }
    // Clear the current input, if requested.
    if (clearCurrent) {
        fInputBuf.clear();
        fInputCurrentChar = 0;
    }
    // If the command is not to be executed, append a space so it won't run together with what the
    // user types next.
    if (not execute) {
        txt.append(' ');
    }
    fInputBuf.insert(fInputCurrentChar, txt);
    fInputCurrentChar += txt.length();
    updateCursorPos();
    if (execute) {
        fEndInputMode(false);
    }
}

QList<const QAction*> HFrame::getGameContextMenuEntries(QMenu& dst)
{
    QList<const QAction*> actions;
    if (fInputMode != InputMode::Normal) {
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
