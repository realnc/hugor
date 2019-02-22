// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QWidget>

#include <QFontMetrics>
#include <QList>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>

#include "happlication.h"

class HFrame;
class QMenu;
class QTimer;

extern HFrame* hFrame;

class HFrame final: public QWidget
{
    Q_OBJECT

private:
    // These values specify the exact input-mode we are in.
    enum class InputMode
    {
        // We aren't in input-mode. We still enqueue key presses though, so they can be retrieved
        // with getNextKey().
        None,

        // Return-terminated input.
        Normal
    };

    // The input-mode we are currently in.
    InputMode input_mode_ = InputMode::None;

    // We have a finished user input.
    bool have_input_ready_ = false;

    // Keypress input queue. If an element is 0, it means the input was a mouse click.
    QQueue<char> key_queue_;

    // Mouse click input queue.
    QQueue<QPoint> click_queue_;

    // The keypress and click queues are also accessed from the engine thread.
    QMutex key_queue_mutex_;
    QMutex click_queue_mutex_;

    // Input buffer.
    QString input_buf_;

    // Position at which we started reading input.
    int input_start_x_ = 0;
    int input_start_y_ = 0;

    // Current editor position, in characters. 0 is the start of the current input buffer string.
    int input_current_char_ = 0;

    // Command history buffer.
    QList<QString> history_;

    // Maximum history capacity.
    int max_hist_cap_ = 200;

    // Current command index in the history. 0 is the most recent one.
    int cur_hist_index_ = 0;

    // Backup of the current, yet to be committed to history, input line. We need this to back up
    // the current input when the user recalls a previous command from the history.
    QString fInputBufBackup;

    // Current colors.
    int fg_color_ = 16;
    int bg_color_ = 17;

    // Current font attributes.
    bool use_fixed_font_ = false;
    bool use_underline_font_ = false;
    bool use_italic_font_ = false;
    bool use_bold_font_ = false;

    // Current font metrics.
    QFontMetrics font_metrics_{QFont()};

    // We render game output into a pixmap first instead or painting directly on the widget. We then
    // draw the pixmap in our paintEvent().
    QPixmap pixmap_{1, 1};

    // We buffer text printed with printText() so that we can draw whole strings rather than single
    // characters at a time.
    QString print_buf_;

    // Position at which to draw the buffered string.
    int flush_pos_x_ = 0;
    int flush_pos_y_ = 0;

    // Position of the text cursor.
    QPoint cursor_pos_{0, 0};

    // Height of the text cursor in pixels.
    float cursor_height_;

    // Text cursor width.
    float cursor_width_ = 1.5f;

    // Last position of the text cursor.
    QPoint last_cursor_pos_{0, 0};

    // Is the text cursor visible?
    bool is_cursor_visible_ = false;

    // Text cursor blink visibility.  Changed by a timer to show/hide the cursor at specific
    // intervals if fCursorVisible is true.
    bool is_blink_visible_ = false;

    // Text cursor blink timer.
    QTimer* blink_timer_;

    // Keeps track of whether the game screen needs updating.
    bool need_screen_update_ = false;

    // We need a small time delay before minimizing when losing focus while in fullscreen mode.
    QTimer* minimize_timer_;

    // Add a keypress to our input queue.
    void enqueueKey(char key, QMouseEvent* e);

    // Set the height of the text cursor in pixels.
    void updateCursorShape();

private slots:
    // Called by the timer to blink the text cursor.
    void blinkCursor();

    // We need to know when the application loses focus entirely so that we can disable keyboard
    // cursor blinking when we lose focus.
    void handleFocusChange(QWidget* old, QWidget* now);

    void handleFocusLost();

    // End line input mode and send the command to the game.
    void endInputMode(bool addToHistory);

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void inputMethodEvent(QInputMethodEvent* e) override;
    void singleKeyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;

signals:
    // Emitted when scrolling or paging up.
    void requestScrollback();

    // Emitted when the escape key is pressed on the keyboard.
    void escKeyPressed();

public:
    HFrame(QWidget* parent);

    // The engine thread waits on this until an input line has been entered.
    QWaitCondition inputLineWaitCond;

    // The engine thread waits on this until a keypress is available.
    QWaitCondition keypressAvailableWaitCond;

    // Start reading an input line.
    void startInput(int xPos, int yPos);

    // Get the most recently entered input line and clear it.
    void getInput(char* buf, size_t buflen);

    // Returns the next character waiting in the queue. If the queue is empty, it will wait for a
    // character to become available. Note: if this returns 0, it means the next "key" is a mouse
    // click; call getNextClick() to get the position of the mouse click.
    int getNextKey();

    QPoint getNextClick();
    bool hasKeyInQueue();

    // Clear a region of the window using the current background color.
    void clearRegion(qreal left, qreal top, qreal right, qreal bottom);

    // Set the current foreground color.
    void setFgColor(int color);

    // Set the current background color.
    void setBgColor(int color);

    void setFontType(int hugoFont);

    const QFontMetrics& currentFontMetrics() const
    {
        return font_metrics_;
    }

    // Print text using the current foreground and background colors. The Text is might not be
    // printed immediately; call flushText() to flush the accumulated text to the screen.
    void printText(const QString& str, int x, int y);

    // Print an image to the screen. The image is printed immediately (no buffering is performed.)
    void printImage(const QImage& img, int x, int y);

    // Change the text cursor position.
    void moveCursorPos(const QPoint& pos)
    {
        cursor_pos_ = pos;
        updateCursorShape();
    }
    // Show/hide the text cursor.
    void setCursorVisible(bool visible)
    {
        is_cursor_visible_ = visible;
    }

    bool isCursorVisible() const
    {
        return is_cursor_visible_;
    }

    void updateCursorPos();

    // Reset cursor blink timer.  This will read the blinking rate from the desktop environment and
    // ajust the blink timer as needed.
    void resetCursorBlinking();

    // Insert text at the current command input position. If 'execute' is set, the command is also
    // executed. If 'clear' is set, the current command will be cleared before adding the text.
    void insertInputText(QString txt, bool execute, bool clearCurrent);

    // Returns a list of current context menu entries set by the game and inserts them into the
    // `dst` menu.
    QList<const QAction*> getGameContextMenuEntries(QMenu& dst);

public slots:
    // Flush any pending text drawing.
    void flushText();

    // Scroll a region of the screen up by 'h' pixels.
    void scrollUp(int left, int top, int right, int bottom, int h);

    // Update the game screen, if needed.
    void updateGameScreen(bool force);
};

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
