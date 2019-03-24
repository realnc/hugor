// This is copyrighted software. More information is at the end of this file.
#include "hframe.h"

#include <QClipboard>
#include <QDebug>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QTextCodec>
#include <QTimer>
#include <QWindow>

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

static qreal dpr()
{
    return hMainWin->windowHandle()->devicePixelRatio();
}

HFrame::HFrame(QWidget* parent)
    : QWidget(parent)
    , cursor_height_(QFontMetrics(hApp->settings()->prop_font).height())
    , blink_timer_(new QTimer(this))
    , minimize_timer_(new QTimer(this))
{
    // We handle player input, so we need to accept focus.
    setFocusPolicy(Qt::WheelFocus);

    connect(blink_timer_, SIGNAL(timeout()), this, SLOT(blinkCursor()));
    resetCursorBlinking();
    // setCursorVisible(true);

    // We need to check whether the application lost focus.
    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)),
            SLOT(handleFocusChange(QWidget*, QWidget*)));

    minimize_timer_->setSingleShot(true);
    connect(minimize_timer_, SIGNAL(timeout()), SLOT(handleFocusLost()));

    // Requesting scrollback simply triggers the scrollback window. Since focus is lost, subsequent
    // scrolling/paging events will work as expected.
    connect(this, SIGNAL(requestScrollback()), hMainWin, SLOT(showScrollback()));

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_OpaquePaintEvent);
    hFrame = this;
}

void HFrame::enqueueKey(char key, QMouseEvent* e)
{
    QMutexLocker mKeyLocker(&key_queue_mutex_);
    QMutexLocker mClickLocker(&click_queue_mutex_);
    // Only allow one keypress or click in the queue. No reason to use a queue for that, but we
    // might still want the possibilty to change this to allow multiple keys/clicks to queue up, if
    // the need for that arises.
    if (key_queue_.empty()) {
        key_queue_.enqueue(key);
        if (e != nullptr) {
            click_queue_.append(e->pos());
        }
    }
    mKeyLocker.unlock();
    mClickLocker.unlock();
    keypressAvailableWaitCond.wakeAll();
}

void HFrame::updateCursorShape()
{
    auto shape = hApp->settings()->cursor_shape;
    float thickness = hApp->settings()->cursor_thickness + 1;

    if (shape == Settings::TextCursorShape::Ibeam) {
        cursor_height_ = font_metrics_.height();
        cursor_width_ = thickness;
        return;
    }

    if (shape == Settings::TextCursorShape::Block) {
        cursor_height_ = font_metrics_.height();
    } else if (shape == Settings::TextCursorShape::Underline) {
        cursor_height_ = thickness;
    }
    auto cur_char = input_buf_[input_current_char_];
    if (cur_char.isNull()) {
        cursor_width_ = font_metrics_.width('_');
    } else {
        cursor_width_ = font_metrics_.width(cur_char);
    }
}

void HFrame::blinkCursor()
{
    is_blink_visible_ = not is_blink_visible_;
    update(cursor_pos_.x() - 2, cursor_pos_.y() - 2, cursor_width_ + 4, font_metrics_.height() + 4);
}

void HFrame::handleFocusChange(QWidget* old, QWidget* now)
{
    if (now == nullptr) {
        // Minimize a bit later, in case we only lose focus for a very short time.
        minimize_timer_->start(200);
    } else if (old == nullptr and now != nullptr) {
        // In case we only lost focus only for a short time, abort any pending minimize operation.
        minimize_timer_->stop();

        // The application window gained focus.  Reset cursor blinking and unmute (just in case we
        // were muted previously.)
        resetCursorBlinking();
        muteSound(false);
        muteVideo(false);
    }
}

void HFrame::handleFocusLost()
{
    // The application window lost focus.  Disable cursor blinking.
    blink_timer_->stop();
#ifdef Q_OS_MAC
    // On the Mac, when applications lose focus the cursor must be disabled.
    if (is_blink_visible_) {
        blinkCursor();
    }
#else
    // On all other systems we assume the cursor must stay visible.
    if (not is_blink_visible_) {
        blinkCursor();
    }
#endif
    // Minimize the application if we're fullscreen (except on OSX.)
    if (hMainWin->isFullScreen()) {
#ifndef Q_OS_MAC
        hMainWin->showMinimized();
#endif
        if (hApp->settings()->mute_when_minimized) {
            muteSound(true);
            muteVideo(true);
        }
    }
}

void HFrame::endInputMode(bool addToHistory)
{
    have_input_ready_ = true;
    input_mode_ = InputMode::None;
    // The current command only needs to be appended to the history if it's not empty and differs
    // from the previous command in the history.
    if (((history_.isEmpty() and not input_buf_.isEmpty())
         or (not history_.isEmpty() and not input_buf_.isEmpty() and input_buf_ != history_.last()))
        and addToHistory) {
        history_.append(input_buf_);
        // If we're about to overflow the max history cap, delete the oldest command from the
        // history.
        if (history_.size() > max_hist_cap_) {
            history_.removeFirst();
        }
    }
    cur_hist_index_ = 0;
    // Make the input text part of the display pixmap.
    printText(input_buf_.toLatin1().constData(), input_start_x_, input_start_y_);
    inputLineWaitCond.wakeAll();
}
#include <cmath>
void HFrame::paintEvent(QPaintEvent* e)
{
    // qDebug(Q_FUNC_INFO);
    QPainter p(this);
    p.setClipRegion(e->region());
    QRectF src_rect(e->rect().topLeft() * dpr(),
                    QSizeF(e->rect().width(), e->rect().height()) * dpr());
    p.drawPixmap(e->rect(), pixmap_, src_rect);

    // Draw our current input. We need to do this here, after the pixmap has already been painted,
    // so that the input gets painted on top. Otherwise, we could not erase text during editing.
    QFont f(use_fixed_font_ ? hApp->settings()->fixed_font : hApp->settings()->prop_font);
    f.setUnderline(use_underline_font_);
    f.setItalic(use_italic_font_);
    f.setBold(use_bold_font_);
    QFontMetrics m(f);
    p.setFont(f);
    if (input_mode_ == InputMode::Normal and not input_buf_.isEmpty()) {
        p.setPen(hugoColorToQt(fg_color_));
        p.setBackgroundMode(Qt::OpaqueMode);
        p.setBackground(QBrush(hugoColorToQt(bg_color_)));
        p.drawText(input_start_x_, input_start_y_ + m.ascent() + 1, input_buf_);
    }

    if (not is_cursor_visible_ or not is_blink_visible_) {
        return;
    }

    // Draw the input caret.
    QPen pen(hugoColorToQt(fg_color_));
    pen.setCapStyle(Qt::FlatCap);
    pen.setCosmetic(false);
    p.setBrush(hugoColorToQt(fg_color_));
    switch (hApp->settings()->cursor_shape) {
    case Settings::TextCursorShape::Ibeam: {
        pen.setWidthF(cursor_width_);
        p.setPen(pen);
        p.drawLine(
            QPointF(cursor_pos_.x() + cursor_width_ / dpr(), cursor_pos_.y()),
            QPointF(cursor_pos_.x() + cursor_width_ / dpr(), cursor_pos_.y() + cursor_height_));
        break;
    }
    case Settings::TextCursorShape::Block:
        pen.setColor(Qt::transparent); // workaround for rounded angles (Qt bug?)
        p.setPen(pen);
        p.drawRect(
            QRectF(cursor_pos_.x() - 1.0, cursor_pos_.y(), cursor_width_ + 2.0, cursor_height_));
        break;
    case Settings::TextCursorShape::Underline:
        pen.setWidthF(cursor_height_);
        p.setPen(pen);
        p.drawLine(
            QPointF(cursor_pos_.x(), cursor_pos_.y() + font_metrics_.height() - 1),
            QPointF(cursor_pos_.x() + cursor_width_, cursor_pos_.y() + font_metrics_.height() - 1));
        break;
    }

    // With a block-shaped cursor, draw the covered character in inverse color.
    if (hApp->settings()->cursor_shape == Settings::TextCursorShape::Block) {
        p.setPen(hugoColorToQt(bg_color_));
        p.setBackgroundMode(Qt::TransparentMode);
        p.drawText(QPointF(cursor_pos_.x(), cursor_pos_.y() + m.ascent() + 1),
                   input_buf_.mid(input_current_char_, 1));
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

    // Adjust the margins so that we get our final size.
    hApp->updateMargins(-1);

    // Create a new pixmap, using the new size and fill it with the default background color.
    QPixmap newPixmap(size() * dpr());
    newPixmap.setDevicePixelRatio(dpr());
    newPixmap.fill(hugoColorToQt(bg_color_));

    // Draw the current pixmap into the new one and use it as our new display.
    QPainter p(&newPixmap);
    p.drawPixmap(0, 0, pixmap_);
    pixmap_ = newPixmap;

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

    if (input_mode_ == InputMode::None) {
        singleKeyPressEvent(e);
        return;
    }

    // Just for having shorter identifiers.
    int& i = input_current_char_;
    QString& buf = input_buf_;

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
        endInputMode(true);
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
        if (cur_hist_index_ == history_.size() or history_.isEmpty()) {
            return;
        }
        // If the current command is new and not in the history yet, remember it so we can bring it
        // back if the user recalls it.
        if (cur_hist_index_ == 0) {
            fInputBufBackup = buf;
        }
        // Recall the previous command from the history.
        buf = history_[history_.size() - 1 - cur_hist_index_];
        ++cur_hist_index_;
        i = buf.length();
    } else if (e->matches(QKeySequence::MoveToNextLine)) {
        // If we're at the latest command, don't do anything.
        if (cur_hist_index_ == 0) {
            return;
        }
        --cur_hist_index_;
        // If the next command is the latest one, it means it's current new command we backed up
        // previously. So restore it. If not, recall the next command from the history.
        if (cur_hist_index_ == 0) {
            buf = fInputBufBackup;
            fInputBufBackup.clear();
        } else {
            buf = history_[history_.size() - cur_hist_index_];
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
        or (input_mode_ == InputMode::Normal and e->commitString().isEmpty())) {
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
    enqueueKey(bytes[0], nullptr);
}

void HFrame::singleKeyPressEvent(QKeyEvent* event)
{
    // qDebug() << Q_FUNC_INFO;
    Q_ASSERT(input_mode_ == InputMode::None);

    switch (event->key()) {
    case 0:
    case Qt::Key_unknown:
        QWidget::keyPressEvent(event);
        return;

    case Qt::Key_Left:
        enqueueKey(8, nullptr);
        break;

    case Qt::Key_Up:
        enqueueKey(11, nullptr);
        break;

    case Qt::Key_Right:
        enqueueKey(21, nullptr);
        break;

    case Qt::Key_Down:
        enqueueKey(10, nullptr);
        break;

    default:
        // If the keypress doesn't correspond to exactly one character, ignore it.
        if (event->text().size() != 1) {
            QWidget::keyPressEvent(event);
            return;
        }
        enqueueKey(event->text().at(0).toLatin1(), nullptr);
    }
}

void HFrame::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    if (input_mode_ == InputMode::None) {
        enqueueKey(0, e);
    }
    e->accept();
}

void HFrame::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (input_mode_ != InputMode::Normal or e->button() != Qt::LeftButton) {
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
    have_input_ready_ = false;
    input_mode_ = InputMode::Normal;
    input_start_x_ = xPos;
    input_start_y_ = yPos;
    input_current_char_ = 0;

    QMutexLocker mKeyLocker(&key_queue_mutex_);
    QMutexLocker mClickLocker(&click_queue_mutex_);
    key_queue_.clear();
    click_queue_.clear();
}

void HFrame::getInput(char* buf, size_t buflen)
{
    Q_ASSERT(buf != nullptr);
    qstrncpy(buf, input_buf_.toLatin1(), buflen);
    input_buf_.clear();
}

int HFrame::getNextKey()
{
    // qDebug() << Q_FUNC_INFO;
    QMutexLocker mLocker(&key_queue_mutex_);
    Q_ASSERT(not key_queue_.isEmpty());
    return key_queue_.dequeue();
}

QPoint HFrame::getNextClick()
{
    QMutexLocker mLocker(&click_queue_mutex_);
    Q_ASSERT(not click_queue_.isEmpty());
    return click_queue_.dequeue();
}

bool HFrame::hasKeyInQueue()
{
    QMutexLocker mLocker(&key_queue_mutex_);
    return not key_queue_.isEmpty();
}

void HFrame::clearRegion(qreal left, qreal top, qreal right, qreal bottom)
{
    // qDebug(Q_FUNC_INFO);
    flushText();
    need_screen_update_ = true;
    if (left == 0 and top == 0 and right == 0 and bottom == 0) {
        pixmap_.fill(hugoColorToQt(bg_color_));
        return;
    }
    QPainter p(&pixmap_);
    QRectF rect(left, top, right - left + 1, bottom - top + 1);
    p.fillRect(rect, hugoColorToQt(bg_color_));

    // If this was a fullscreen clear, then also clear the margin color.
    if (rect == pixmap_.rect()) {
        hApp->updateMargins(bg_color_);
    }
}

void HFrame::setFgColor(int color)
{
    flushText();
    fg_color_ = color;
}

void HFrame::setBgColor(int color)
{
    flushText();
    bg_color_ = color;
}

void HFrame::setFontType(int hugoFont)
{
    flushText();
    use_fixed_font_ = not(hugoFont & PROP_FONT);
    use_underline_font_ = hugoFont & UNDERLINE_FONT;
    use_italic_font_ = hugoFont & ITALIC_FONT;
    use_bold_font_ = hugoFont & BOLD_FONT;

    QFont f(use_fixed_font_ ? hApp->settings()->fixed_font : hApp->settings()->prop_font);
    f.setUnderline(use_underline_font_);
    f.setItalic(use_italic_font_);
    f.setBold(use_bold_font_);
    font_metrics_ = QFontMetrics(f, &pixmap_);

    // Adjust text caret for new font.
    updateCursorShape();
}

void HFrame::printText(const QString& str, int x, int y)
{
    if (print_buf_.isEmpty()) {
        flush_pos_x_ = x;
        flush_pos_y_ = y;
    }
    print_buf_ += str;
}

void HFrame::printImage(const QImage& img, int x, int y)
{
    flushText();
    QPainter p(&pixmap_);
    p.drawImage(x, y, img);
    need_screen_update_ = true;
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
    pixmap_.scroll(0, -h * dpr(), left * dpr(), top * dpr(), (right - left) * dpr(),
                   (bottom - top) * dpr(), &exposed);
    need_screen_update_ = true;

    // Fill exposed region.
    const QRect& r = exposed.boundingRect();
    clearRegion(r.left() / dpr(), r.top() / dpr(), (r.left() + r.width()) / dpr(),
                (r.top() + r.bottom()) / dpr());

    if (hApp->settings()->soft_text_scrolling) {
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
    if (print_buf_.isEmpty()) {
        return;
    }

    QFont f(use_fixed_font_ ? hApp->settings()->fixed_font : hApp->settings()->prop_font);
    f.setUnderline(use_underline_font_);
    f.setItalic(use_italic_font_);
    f.setBold(use_bold_font_);
    QPainter p(&pixmap_);

    // Manually fill the text background before drawing the text. We need this because
    // QPainter::drawText() will not fill the whole height of the line.
    p.save();
    QPen pen;
    pen.setColor(Qt::transparent); // workaround for rounded angles (Qt bug?)
    pen.setCapStyle(Qt::FlatCap);
    pen.setCosmetic(false);
    p.setPen(pen);
    p.setBrush(hugoColorToQt(bg_color_));
    p.drawRect(flush_pos_x_, flush_pos_y_ + 1, currentFontMetrics().width(print_buf_),
               currentFontMetrics().lineSpacing());
    p.restore();

    p.setFont(f);
    p.setPen(hugoColorToQt(fg_color_));
    p.drawText(flush_pos_x_, flush_pos_y_ + currentFontMetrics().ascent(), print_buf_);
    print_buf_.clear();
    need_screen_update_ = true;
}

void HFrame::updateGameScreen(bool force)
{
    flushText();
    if (need_screen_update_ or force) {
        // qDebug(Q_FUNC_INFO);
        hApp->updateMargins(bg_color_);
        need_screen_update_ = false;
        hApp->marginWidget()->update();
        update();
    }
}

void HFrame::updateCursorPos()
{
    // Reset the blink timer.
    if (blink_timer_->isActive()) {
        blink_timer_->start();
    }

    // Blink-out first to ensure the cursor won't stay visible at the previous position after we
    // move it.
    if (is_blink_visible_) {
        blinkCursor();
    }

    // Blink-in.
    if (not is_blink_visible_) {
        blinkCursor();
    }

    int xOffs = currentFontMetrics().width(input_buf_.left(input_current_char_));
    moveCursorPos(QPoint(input_start_x_ + xOffs, input_start_y_));

    // Blink-in.
    if (not is_blink_visible_) {
        blinkCursor();
    }
    update();
}

void HFrame::resetCursorBlinking()
{
    // Start the timer unless cursor blinking is disabled.
    if (QApplication::cursorFlashTime() > 1) {
        blink_timer_->start(QApplication::cursorFlashTime() / 2);
    }
}

void HFrame::insertInputText(QString txt, bool execute, bool clearCurrent)
{
    if (input_mode_ != InputMode::Normal) {
        return;
    }
    // Clear the current input, if requested.
    if (clearCurrent) {
        input_buf_.clear();
        input_current_char_ = 0;
    }
    // If the command is not to be executed, append a space so it won't run together with what the
    // user types next.
    if (not execute) {
        txt.append(' ');
    }
    input_buf_.insert(input_current_char_, txt);
    input_current_char_ += txt.length();
    updateCursorPos();
    if (execute) {
        endInputMode(false);
    }
}

QList<const QAction*> HFrame::getGameContextMenuEntries(QMenu& dst)
{
    QList<const QAction*> actions;
    if (input_mode_ != InputMode::Normal) {
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
