// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QMainWindow>

class AboutDialog;
class ConfDialog;
class HMainWindow;
class HScrollbackWindow;
class QErrorMessage;

extern HMainWindow* hMainWin;

class HMainWindow final: public QMainWindow
{
    Q_OBJECT

private:
    QErrorMessage* error_msg_ = nullptr;
    ConfDialog* conf_dialog_ = nullptr;
    AboutDialog* about_dialog_ = nullptr;
    HScrollbackWindow* scrollback_window_ = nullptr;
    QAction* preferences_action_ = nullptr;
    QAction* fullscreen_action_ = nullptr;
    QAction* scrollback_action_ = nullptr;
    bool is_menubar_visible_ = true;
    QIcon fullscreen_enter_icon_;
    QIcon fullscreen_exit_icon_;

    void fullscreenAdjust();

private slots:
    void showConfDialog();
    void hideConfDialog();
    void showAbout();
    void hideAbout();

protected:
    void closeEvent(QCloseEvent* e) override;
    void changeEvent(QEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

public:
    HMainWindow(QWidget* parent);

    void hideMenuBar();
    void showMenuBar();

    QErrorMessage* errorMsgObj() const
    {
        return error_msg_;
    }

    void setScrollbackFont(const QFont& font);

public slots:
    void appendToScrollback(const QByteArray& str);
    void showScrollback();
    void hideScrollback();
    void toggleFullscreen();
    void setFullscreen(bool f);
};

/* Copyright (C) 2011-2018 Nikos Chantziaras
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
