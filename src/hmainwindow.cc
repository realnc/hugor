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
#include <QMenuBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QLayout>
#include <QTextEdit>
#include <QTextCodec>
#include <QScrollBar>
#include <QLabel>
#include <QWindowStateChangeEvent>
#include <QErrorMessage>

#include "hmainwindow.h"
#include "happlication.h"
#include "hframe.h"
#include "hscrollback.h"
#include "hmarginwidget.h"
#include "confdialog.h"
#include "aboutdialog.h"
#include "settings.h"
#include "hugodefs.h"
extern "C" {
#include "heheader.h"
}

HMainWindow* hMainWin = nullptr;


HMainWindow::HMainWindow( QWidget* parent )
    : QMainWindow(parent)
    , fFullscreenEnterIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")))
    , fFullscreenExitIcon(QIcon::fromTheme(QStringLiteral("view-restore")))
{
    Q_ASSERT(hMainWin == nullptr);

    // We make our menu bar parentless so it will be shared by all our windows
    // in Mac OS X.
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
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(fShowConfDialog()));
    fPreferencesAction = act;

    // "View" menu.
    menu = menuBar->addMenu(tr("&View"));
    act = new QAction(tr("Show &Scrollback"), this);
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(showScrollback()));
    fScrollbackAction = act;

#ifdef Q_OS_MAC
    act = new QAction(tr("Enter &Full Screen"), this);
#else
    act = new QAction(tr("&Fullscreen Mode"), this);
    act->setCheckable(true);
#endif
    act->setIcon(fFullscreenEnterIcon);
    QList<QKeySequence> keySeqList;
#ifdef Q_OS_MAC
    keySeqList.append(QKeySequence("Meta+Ctrl+F"));
#elif defined(Q_OS_WIN)
    keySeqList.append(QKeySequence("F11"));
    keySeqList.append(QKeySequence("Alt+Return"));
    keySeqList.append(QKeySequence("Alt+Enter"));
#else
    if (hApp->desktopIsGnome()) {
        keySeqList.append(QKeySequence("Ctrl+F11"));
    } else {
        // Assume KDE.
        keySeqList.append(QKeySequence("F11"));
        keySeqList.append(QKeySequence("Shift+Ctrl+F"));
    }
#endif
    act->setShortcuts(keySeqList);
    act->setShortcutContext(Qt::ApplicationShortcut);
    menu->addAction(act);
    addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(toggleFullscreen()));
    fFullscreenAction = act;

    // "Help" menu.
    menu = menuBar->addMenu(tr("&Help"));
    act = new QAction(tr("A&bout Hugor"), this);
    act->setIcon(QIcon::fromTheme(QString::fromLatin1("help-about")));
    menu->addAction(act);
    connect(act, SIGNAL(triggered()), SLOT(fShowAbout()));

    fScrollbackWindow = new HScrollbackWindow(nullptr);

    // Use a sane minimum size; by default Qt would allow us to be resized
    // to almost zero.
    setMinimumSize(240, 180);

    fErrorMsg = new QErrorMessage(this);
    fErrorMsg->setWindowTitle(HApplication::applicationName());

    hMainWin = this;
}


void
HMainWindow::fFullscreenAdjust()
{
    if (isFullScreen()) {
        fFullscreenAction->setIcon(fFullscreenExitIcon);
#ifdef Q_OS_MAC
        fFullscreenAction->setText("Exit Full Screen");
#else
        fFullscreenAction->setChecked(true);
        menuBar()->hide();
#endif
    } else {
        fFullscreenAction->setIcon(fFullscreenEnterIcon);
#ifdef Q_OS_MAC
        fFullscreenAction->setText("Enter Full Screen");
#else
        fFullscreenAction->setChecked(false);
        if (fMenuBarVisible) {
            menuBar()->show();
        }
#endif
    }
    hApp->updateMargins(-1);
    hFrame->updateGameScreen(true);
}


void
HMainWindow::fShowConfDialog()
{
    // If the dialog is already open, simply activate and raise it.
    if (fConfDialog != nullptr) {
        fConfDialog->activateWindow();
        fConfDialog->raise();
        return;
    }
    fConfDialog = new ConfDialog(this);
    fConfDialog->setWindowTitle(HApplication::applicationName() + ' ' + tr("Preferences"));
    connect(fConfDialog, SIGNAL(finished(int)), this, SLOT(fHideConfDialog()));
    fConfDialog->show();
}


void
HMainWindow::fHideConfDialog()
{
    if (fConfDialog != nullptr) {
        fConfDialog->deleteLater();
        fConfDialog = nullptr;
    }
}


void
HMainWindow::fShowAbout()
{
    // If the dialog is already open, simply activate and raise it.
    if (fAboutDialog != nullptr) {
        fAboutDialog->activateWindow();
        fAboutDialog->raise();
        return;
    }

    fAboutDialog = new AboutDialog(this);
    connect(fAboutDialog, SIGNAL(finished(int)), SLOT(fHideAbout()));
    fAboutDialog->show();
}


void
HMainWindow::fHideAbout()
{
    if (fAboutDialog != nullptr) {
        fAboutDialog->deleteLater();
        fAboutDialog = nullptr;
    }
}


void
HMainWindow::showScrollback()
{
    // Make sure the mouse cursor is visible.
    hApp->marginWidget()->unsetCursor();

    // If no overlay was requested and the scrollback is currently in its
    // overlay mode, make it a regular window again.
    if (not hApp->settings()->overlayScrollback) {
        hideScrollback();
        // If a widget has no parent, it will be displayed in a new window,
        fScrollbackWindow->setParent(nullptr);
        fScrollbackWindow->show();
        fScrollbackWindow->activateWindow();
        fScrollbackWindow->raise();
        return;
    }

    // If the overlay is already visible, no need to do anything.
    if (hApp->marginWidget()->layout()->indexOf(fScrollbackWindow) >= 0) {
        return;
    }

    // We need to overlay the scrollback. Remove the game window from view and
    // replace it with the scrollback window if we haven't already done so.
    hApp->marginWidget()->removeWidget(hFrame);
    hApp->marginWidget()->setContentsMargins(0, 0, 0, 0);
    hApp->marginWidget()->addWidget(fScrollbackWindow);
    hApp->marginWidget()->setPalette(QApplication::palette(hApp->marginWidget()));

    // Add a banner at the top so the user knows how to exit
    // scrollback mode.
    QLabel* banner = new QLabel(fScrollbackWindow);
    banner->setAlignment(Qt::AlignCenter);
    banner->setContentsMargins(0, 3, 0, 3);

    // Display the banner in reverse colors so that it's visible
    // regardless of the desktop's color theme.
    QPalette pal = banner->palette();
    pal.setColor(QPalette::Window, fScrollbackWindow->palette().color(QPalette::WindowText));
    pal.setBrush(QPalette::WindowText, fScrollbackWindow->palette().color(QPalette::Base));
    banner->setPalette(pal);
    banner->setAutoFillBackground(true);

    // Make the informational message clickable by displaying it as a
    // link that closes the scrollback when clicked.
    QString bannerText("<html><style>a {text-decoration: none; color: ");
    bannerText += pal.color(QPalette::WindowText).name()
               +  ";}</style><body>Scrollback (<a href='close'>Esc, "
               +  QKeySequence(QKeySequence::Close).toString(QKeySequence::NativeText)
               +  " or click here to exit</a>)</body></html>";
    banner->setText(bannerText);
    connect(banner, SIGNAL(linkActivated(QString)), SLOT(hideScrollback()));

    hApp->marginWidget()->setBannerWidget(banner);
    hFrame->hide();
    fScrollbackWindow->show();
    fScrollbackWindow->setFocus();
}


void
HMainWindow::hideScrollback()
{
    // We only have cleanup work if a scrollback overlay is currently active.
    if (hApp->marginWidget()->layout()->indexOf(fScrollbackWindow) < 0) {
        return;
    }

    hApp->marginWidget()->setBannerWidget(nullptr);
    hApp->marginWidget()->removeWidget(fScrollbackWindow);
    fScrollbackWindow->hide();
    hApp->marginWidget()->addWidget(hFrame);
    hApp->updateMargins(::bgcolor);
    hFrame->show();
    hFrame->setFocus();
}


void
HMainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        if (hApp->settings()->isMaximized) {
            showMaximized();
        }
        hApp->settings()->isFullscreen = false;
    } else {
        // Remember our windowed size in case we quit while in fullscreen.
        hApp->settings()->appSize = size();
        hApp->settings()->isFullscreen = true;
        hApp->settings()->isMaximized = isMaximized();
        showFullScreen();
    }
    hApp->settings()->saveToDisk();
    fFullscreenAdjust();
}


void
HMainWindow::setFullscreen( bool f )
{
    if ((f and isFullScreen()) or (not f and not isFullScreen())) {
        return;
    }
    toggleFullscreen();
}


void
HMainWindow::closeEvent( QCloseEvent* e )
{
    if (not hApp->gameRunning()) {
        QMainWindow::closeEvent(e);
        return;
    }

    QMessageBox* msgBox = new QMessageBox(QMessageBox::Question,
                                          tr("Quit") + ' ' + HApplication::applicationName(),
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
        hApp->settings()->saveToDisk();
        closeSoundEngine();
        hApp->terminateEngineThread();
        exit(0);
    } else {
        e->ignore();
    }
}


void
HMainWindow::changeEvent( QEvent* e )
{
    // Don't do anything special if the window state was not changed by the
    // environment.
    if (e->type() != QEvent::WindowStateChange or not e->spontaneous()) {
        QMainWindow::changeEvent(e);
        return;
    }

    auto* chEv = static_cast<QWindowStateChangeEvent*>(e);

    if ((chEv->oldState().testFlag(Qt::WindowFullScreen) and not isFullScreen())
        or (not chEv->oldState().testFlag(Qt::WindowFullScreen) and isFullScreen()))
    {
        fFullscreenAdjust();
    } else if (chEv->oldState().testFlag(Qt::WindowMinimized) and not isMinimized()) {
        muteSound(false);
        muteVideo(false);
    } else if (hApp->settings()->muteWhenMinimized
               and not chEv->oldState().testFlag(Qt::WindowMinimized) and isMinimized())
    {
        muteSound(true);
        muteVideo(true);
    }
    QMainWindow::changeEvent(e);
}


void
HMainWindow::contextMenuEvent( QContextMenuEvent* e )
{
    QMenu menu(this);
    QList<const QAction*> actions = hFrame->getGameContextMenuEntries(menu);
    if (not actions.isEmpty()) {
        menu.addSeparator();
    }
    menu.addAction(fPreferencesAction);
    menu.addAction(fScrollbackAction);
    menu.addAction(fFullscreenAction);
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


void
HMainWindow::appendToScrollback( const QByteArray& str )
{
    if (str.isEmpty()) {
        return;
    }

    fScrollbackWindow->moveCursor(QTextCursor::End);
    fScrollbackWindow->insertPlainText(hApp->hugoCodec()->toUnicode(str));
    fScrollbackWindow->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}


void
HMainWindow::hideMenuBar()
{
    fMenuBarVisible = false;
    menuBar()->hide();
}


void
HMainWindow::showMenuBar()
{
    fMenuBarVisible = true;
    menuBar()->show();
}


void
HMainWindow::setScrollbackFont( const QFont& font )
{
    fScrollbackWindow->setFont(font);
}
