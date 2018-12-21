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
#include "hscrollback.h"

#include <QKeyEvent>

#include "happlication.h"
#include "hmainwindow.h"
#include "settings.h"

HScrollbackWindow::HScrollbackWindow(QWidget* parent)
    : QTextEdit(parent)
{
    setWindowTitle(HApplication::applicationName() + ' ' + "Scrollback");
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setFrameStyle(QFrame::NoFrame | QFrame::Plain);
    setFont(hApp->settings()->scrollback_font);
    // Don't allow the scrollbuffer to grow forever; limit it to fMaximumBlockCount lines.
    document()->setMaximumBlockCount(max_block_count_);
    resize(initial_width_, initial_height_);
}

void HScrollbackWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->matches(QKeySequence::Close) or e->key() == Qt::Key_Escape) {
        close();
        hMainWin->activateWindow();
        hMainWin->raise();
        e->accept();
    } else {
        QTextEdit::keyPressEvent(e);
    }
}

void HScrollbackWindow::closeEvent(QCloseEvent* e)
{
    QTextEdit::closeEvent(e);
    hMainWin->hideScrollback();
}
