// This is copyrighted software. More information is at the end of this file.
#include "hmainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QErrorMessage>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QWindowStateChangeEvent>

#include "aboutdialog.h"
#include "confdialog.h"
#include "happlication.h"
extern "C" {
#include "heheader.h"
}
#include "hframe.h"
#include "hmarginwidget.h"
#include "hscrollback.h"
#include "hugodefs.h"
#include "settings.h"
#include "util.h"

HMainWindow* hMainWin = nullptr;

HMainWindow::HMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , fullscreen_enter_icon_(QIcon::fromTheme(QStringLiteral("view-fullscreen")))
    , fullscreen_exit_icon_(QIcon::fromTheme(QStringLiteral("view-restore")))
{
    Q_ASSERT(hMainWin == nullptr);

    // We make our menu bar parentless so it will be shared by all our windows in Mac OS X.
    auto* menuBar = new QMenuBar(nullptr);
#ifndef Q_OS_MAC
    setMenuBar(menuBar);
#endif

    QMenu* menu;
    QAction* act;

    // "Edit" menu.
    menu = menuBar->addMenu(tr("&Edit"));
    act = new QAction(tr("&Preferences..."), this);
    act->setIcon(QIcon::fromTheme(QString::fromLatin1("configure")));
    act->setShortcuts(QKeySequence::Preferences);
    act->setMenuRole(QAction::PreferencesRole);
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(showConfDialog()));
    preferences_action_ = act;

    // "View" menu.
    menu = menuBar->addMenu(tr("&View"));
    act = new QAction(tr("Show &Scrollback"), this);
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(showScrollback()));
    scrollback_action_ = act;

    act = new QAction(tr("Enter &Full Screen"), this);
#ifndef Q_OS_MACOS
    act->setCheckable(true);
#endif
    auto shortcuts = QKeySequence::keyBindings(QKeySequence::FullScreen);
    // Also allow alt+enter/alt+return as a fullscreen toggle even if the platform normally doesn't
    // use that.
    bool alt_enter_found = false;
    bool alt_return_found = false;
    for (const auto& i : qAsConst(shortcuts)) {
        if (i.matches({"alt+enter"})) {
            alt_enter_found = true;
        } else if (i.matches({"alt+return"})) {
            alt_return_found = true;
        }
    }
    if (not alt_enter_found) {
        shortcuts += QKeySequence("alt+enter");
    }
    if (not alt_return_found) {
        shortcuts += QKeySequence("alt+return");
    }
    act->setShortcuts(shortcuts);
    act->setIcon(fullscreen_enter_icon_);
    act->setShortcutContext(Qt::ApplicationShortcut);
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(toggleFullscreen()));
    fullscreen_action_ = act;

    // "Help" menu.
    menu = menuBar->addMenu(tr("&Help"));
    act = new QAction(tr("A&bout Hugor"), this);
    act->setIcon(QIcon::fromTheme(QString::fromLatin1("help-about")));
    act->setMenuRole(QAction::AboutRole);
    menu->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(showAbout()));

    scrollback_window_ = new HScrollbackWindow(nullptr);

    // Use a sane minimum size; by default Qt would allow us to be resized to almost zero.
    setMinimumSize(240, 180);

    error_msg_ = new QErrorMessage(this);
    error_msg_->setWindowTitle(HApplication::applicationName());

    hMainWin = this;
}

void HMainWindow::fullscreenAdjust()
{
    if (isFullScreen()) {
        fullscreen_action_->setIcon(fullscreen_exit_icon_);
        fullscreen_action_->setText("Exit &Full Screen");
#ifndef Q_OS_MACOS
        fullscreen_action_->setChecked(true);
        menuBar()->hide();
#endif
    } else {
        fullscreen_action_->setIcon(fullscreen_enter_icon_);
        fullscreen_action_->setText("Enter &Full Screen");
#ifndef Q_OS_MACOS
        fullscreen_action_->setChecked(false);
        if (is_menubar_visible_) {
            menuBar()->show();
        }
#endif
    }
    hApp->updateMargins(-1);
    hFrame->updateGameScreen(true);
}

void HMainWindow::showConfDialog()
{
    // If the dialog is already open, simply activate and raise it.
    if (conf_dialog_ != nullptr) {
        conf_dialog_->activateWindow();
        conf_dialog_->raise();
        return;
    }
    conf_dialog_ = new ConfDialog(this);
    conf_dialog_->setWindowTitle(HApplication::applicationName() + ' ' + tr("Preferences"));
    connect(conf_dialog_, SIGNAL(finished(int)), this, SLOT(hideConfDialog()));
    conf_dialog_->show();
}

void HMainWindow::hideConfDialog()
{
    if (conf_dialog_ != nullptr) {
        conf_dialog_->deleteLater();
        conf_dialog_ = nullptr;
    }
}

void HMainWindow::showAbout()
{
    // If the dialog is already open, simply activate and raise it.
    if (about_dialog_ != nullptr) {
        about_dialog_->activateWindow();
        about_dialog_->raise();
        return;
    }

    about_dialog_ = new AboutDialog(this);
    connect(about_dialog_, SIGNAL(finished(int)), SLOT(hideAbout()));
    about_dialog_->show();
}

void HMainWindow::hideAbout()
{
    if (about_dialog_ != nullptr) {
        about_dialog_->deleteLater();
        about_dialog_ = nullptr;
    }
}

void HMainWindow::showScrollback()
{
    // Make sure the mouse cursor is visible.
    hApp->marginWidget()->unsetCursor();

    // If no overlay was requested and the scrollback is currently in its overlay mode, make it a
    // regular window again.
    if (not hApp->settings()->overlay_scrollback) {
        hideScrollback();
        // If a widget has no parent, it will be displayed in a new window,
        scrollback_window_->setParent(nullptr);
        scrollback_window_->show();
        scrollback_window_->activateWindow();
        scrollback_window_->raise();
        return;
    }

    // If the overlay is already visible, no need to do anything.
    if (hApp->marginWidget()->layout()->indexOf(scrollback_window_) >= 0) {
        return;
    }

    // We need to overlay the scrollback. Remove the game window from view and replace it with the
    // scrollback window if we haven't already done so.
    hApp->marginWidget()->removeWidget(hFrame);
    hApp->marginWidget()->setContentsMargins(0, 0, 0, 0);
    hApp->marginWidget()->addWidget(scrollback_window_);
    hApp->marginWidget()->setPalette(QApplication::palette(hApp->marginWidget()));

    // Add a banner at the top so the user knows how to exit scrollback mode.
    QLabel* banner = new QLabel(scrollback_window_);
    banner->setAlignment(Qt::AlignCenter);
    banner->setContentsMargins(0, 3, 0, 3);

    // Display the banner in reverse colors so that it's visible regardless of the desktop's color
    // theme.
    QPalette pal = banner->palette();
    pal.setColor(QPalette::Window, scrollback_window_->palette().color(QPalette::WindowText));
    pal.setBrush(QPalette::WindowText, scrollback_window_->palette().color(QPalette::Base));
    banner->setPalette(pal);
    banner->setAutoFillBackground(true);

    // Make the informational message clickable by displaying it as a link that closes the
    // scrollback when clicked.
    QString bannerText("<html><style>a {text-decoration: none; color: ");
    bannerText += pal.color(QPalette::WindowText).name()
                  + ";}</style><body>Scrollback (<a href='close'>Esc, "
                  + QKeySequence(QKeySequence::Close).toString(QKeySequence::NativeText)
                  + " or click here to exit</a>)</body></html>";
    banner->setText(bannerText);
    connect(banner, SIGNAL(linkActivated(QString)), SLOT(hideScrollback()));

    hApp->marginWidget()->setBannerWidget(banner);
    hFrame->hide();
    scrollback_window_->show();
    scrollback_window_->setFocus();
}

void HMainWindow::hideScrollback()
{
    // We only have cleanup work if a scrollback overlay is currently active.
    if (hApp->marginWidget()->layout()->indexOf(scrollback_window_) < 0) {
        return;
    }

    hApp->marginWidget()->setBannerWidget(nullptr);
    hApp->marginWidget()->removeWidget(scrollback_window_);
    scrollback_window_->hide();
    hApp->marginWidget()->addWidget(hFrame);
    hApp->updateMargins(::bgcolor);
    hFrame->show();
    hFrame->setFocus();
}

void HMainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        if (hApp->settings()->is_maximized) {
            showMaximized();
        }
        hApp->settings()->is_fullscreen = false;
    } else {
        // Remember our windowed size in case we quit while in fullscreen.
        hApp->settings()->app_size = size();
        hApp->settings()->is_fullscreen = true;
        hApp->settings()->is_maximized = isMaximized();
        showFullScreen();
    }
    hApp->settings()->saveToDisk();
    fullscreenAdjust();
}

void HMainWindow::setFullscreen(bool f)
{
    if ((f and isFullScreen()) or (not f and not isFullScreen())) {
        return;
    }
    toggleFullscreen();
}

void HMainWindow::closeEvent(QCloseEvent* e)
{
    if (not hApp->gameRunning()) {
        if (conf_dialog_ != nullptr) {
            conf_dialog_->close();
        }
        QMainWindow::closeEvent(e);
        return;
    }

    QMessageBox* msgBox =
        new QMessageBox(QMessageBox::Question, tr("Quit") + ' ' + HApplication::applicationName(),
                        tr("Abandon the story and quit the application?"),
                        QMessageBox::Yes | QMessageBox::Cancel, this);
    msgBox->setDefaultButton(QMessageBox::Cancel);
    msgBox->setInformativeText(tr("Any unsaved progress in the story will be lost."));
#ifdef Q_OS_MAC
    msgBox->setIconPixmap(QPixmap(":/he_32-bit_72x72.png"));
    // This presents the dialog as a sheet in OS X.
    msgBox->setWindowModality(Qt::WindowModal);
#endif

    if (msgBox->exec() == QMessageBox::Yes) {
        if (conf_dialog_ != nullptr) {
            conf_dialog_->close();
        }
        hApp->settings()->saveToDisk();
        closeVideoEngine();
        closeSoundEngine();
        hApp->terminateEngineThread();
        exit(0);
    } else {
        e->ignore();
    }
}

void HMainWindow::changeEvent(QEvent* e)
{
    // Don't do anything special if the window state was not changed by the environment.
    if (e->type() != QEvent::WindowStateChange or not e->spontaneous()) {
        QMainWindow::changeEvent(e);
        return;
    }

    auto* chEv = static_cast<QWindowStateChangeEvent*>(e);

    if ((chEv->oldState().testFlag(Qt::WindowFullScreen) and not isFullScreen())
        or (not chEv->oldState().testFlag(Qt::WindowFullScreen) and isFullScreen())) {
        fullscreenAdjust();
    } else if (chEv->oldState().testFlag(Qt::WindowMinimized) and not isMinimized()) {
        muteSound(false);
        muteVideo(false);
    } else if (hApp->settings()->mute_when_minimized
               and not chEv->oldState().testFlag(Qt::WindowMinimized) and isMinimized()) {
        muteSound(true);
        muteVideo(true);
    }
    QMainWindow::changeEvent(e);
}

void HMainWindow::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu menu(this);
    QList<const QAction*> actions = hFrame->getGameContextMenuEntries(menu);
    if (not actions.isEmpty()) {
        menu.addSeparator();
    }
    menu.addAction(preferences_action_);
    menu.addAction(scrollback_action_);
    menu.addAction(fullscreen_action_);
    const QAction* selectedAction = menu.exec(e->globalPos());
    e->accept();
    if ((selectedAction == nullptr) or not actions.contains(selectedAction)) {
        return;
    }
    QString text(selectedAction->text());
    bool execute = false;
    if (text.endsWith("...")) {
        text.truncate(text.length() - 3);
    } else {
        execute = true;
    }
    hFrame->insertInputText(text, execute, true);
}

void HMainWindow::appendToScrollback(const QByteArray& str)
{
    if (str.isEmpty()) {
        return;
    }

    scrollback_window_->moveCursor(QTextCursor::End);
    scrollback_window_->insertPlainText(hApp->hugoCodec()->toUnicode(str));
    scrollback_window_->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void HMainWindow::hideMenuBar()
{
    is_menubar_visible_ = false;
    menuBar()->hide();
}

void HMainWindow::showMenuBar()
{
    is_menubar_visible_ = true;
    menuBar()->show();
}

void HMainWindow::setScrollbackFont(const QFont& font)
{
    scrollback_window_->setFont(font);
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
