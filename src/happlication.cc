// This is copyrighted software. More information is at the end of this file.
#include "happlication.h"

#include <QDebug>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QStatusBar>
#include <QStyle>
#include <QTextCodec>
#include <utility>

#include "enginerunner.h"
extern "C" {
#include "heheader.h"
}
#include "hframe.h"
#include "hmainwindow.h"
#include "hmarginwidget.h"
#include "hugodefs.h"
#include "hugohandlers.h"
#include "settings.h"
#include "settingsoverrides.h"
#include "videoplayer.h"

HApplication* hApp = nullptr;

static void addBundledFonts(const QString& path)
{
    QDir dir(QApplication::applicationDirPath());
    if (not dir.cd(path)) {
        return;
    }

    dir.setNameFilters({"*.ttf", "*.ttc", "*.otf"});
    dir.setFilter(QDir::Files | QDir::Readable);
    QFontDatabase::removeAllApplicationFonts();
    const auto& absolute_path = dir.absolutePath() + '/';
    const auto& font_files = dir.entryList();
    for (const auto& font_file : font_files) {
        QFontDatabase::addApplicationFont(absolute_path + font_file);
    }
}

HApplication::HApplication(int& argc, char* argv[], const char* appName, const char* appVersion,
                           const char* orgName, const char* orgDomain)
    : QApplication(argc, argv)
    , hugo_codec_(QTextCodec::codecForName("Windows-1252"))
{
    // qDebug() << Q_FUNC_INFO;
    Q_ASSERT(hApp == nullptr);

    // Check if a config file with the same basename as ours exists in our directory. If yes, we
    // will override default settings from it.
    QString cfgFname = QApplication::applicationDirPath();
    if (not cfgFname.endsWith('/')) {
        cfgFname += '/';
    }
    cfgFname += QFileInfo(QApplication::applicationFilePath()).baseName();
    cfgFname += ".cfg";
    if (not QFileInfo::exists(cfgFname)) {
        cfgFname.clear();
    }
    SettingsOverrides* settOvr;
    if (cfgFname.isEmpty()) {
        settOvr = nullptr;
    } else {
        settOvr = new SettingsOverrides(cfgFname);
    }

    if (settOvr and not settOvr->font_dir.isEmpty()) {
        addBundledFonts(settOvr->font_dir);
    }

    HApplication::setApplicationName(QString::fromLatin1(appName));
    HApplication::setApplicationVersion(QString::fromLatin1(appVersion));
    HApplication::setOrganizationName(QString::fromLatin1(orgName));
    HApplication::setOrganizationDomain(QString::fromLatin1(orgDomain));

    // Possibly override application and organization names.
    if (settOvr != nullptr) {
        // Make sure that appName and authorName are either both set or unset. This avoids mixing up
        // the system file/registry paths for the settings with our default ones.
        if ((settOvr->app_name.isEmpty() and not settOvr->author_name.isEmpty())
            or (settOvr->author_name.isEmpty() and not settOvr->app_name.isEmpty())) {
            settOvr->app_name.clear();
            settOvr->author_name.clear();
        }

        if (not settOvr->app_name.isEmpty()) {
            HApplication::setApplicationName(settOvr->app_name);
        }
        if (not settOvr->author_name.isEmpty()) {
            HApplication::setOrganizationName(settOvr->author_name);
        }
    }

#ifdef Q_OS_UNIX
    // Detect whether we're running in Gnome.
    auto layoutPolicy = QDialogButtonBox::ButtonLayout(
        QApplication::style()->styleHint(QStyle::SH_DialogButtonLayout));
    if (layoutPolicy == QDialogButtonBox::GnomeLayout) {
        is_desktop_gnome = true;
    } else {
        is_desktop_gnome = false;
    }
#endif

    // Load our persistent settings.
    settings_.loadFromDisk(settOvr);

    // Apply the smart formatting setting.
    smartformatting = settings_.smart_formatting;

    // Set our global pointer.
    hApp = this;

    // Create our main application window.
    main_win_ = std::make_unique<HMainWindow>(nullptr);
    main_win_->setWindowTitle(HApplication::applicationName());

    // Disable screen updates until we're actually ready to run a game. This prevents screen flicker
    // due to the resizing and background color changes if we're starting in fullscreen mode.
    main_win_->setUpdatesEnabled(false);

    // This widget provides margins for fFrameWin.
    margin_widget_ = new HMarginWidget(main_win_.get());

    frame_win_ = new HFrame(margin_widget_);
    margin_widget_->addWidget(frame_win_);
    updateMargins(-1);
    main_win_->setCentralWidget(margin_widget_);

    if ((settOvr != nullptr) and settOvr->hide_menubar) {
        main_win_->hideMenuBar();
    }

    // Restore the application's size.
    main_win_->resize(settings_.app_size);

    // Set application window icon, unless we're on OS X where the bundle icon is used.
#ifndef Q_OS_MAC
    HApplication::setWindowIcon(QIcon(":/he_32-bit_48x48.png"));
#endif
    delete settOvr;
}

HApplication::~HApplication()
{
    // qDebug() << Q_FUNC_INFO;
    Q_ASSERT(hApp != nullptr);
    settings_.saveToDisk();
    // We're being destroyed, so our global pointer is no longer valid.
    hApp = nullptr;
}

void HApplication::runGame()
{
    if (next_game_.isEmpty()) {
        // Nothing to run.
        return;
    }

    while (not next_game_.isEmpty()) {
        QFileInfo finfo(QFileInfo(next_game_).absoluteFilePath());
        next_game_.clear();

        // Remember the directory of the game.
        settings_.last_file_open_dir = finfo.absolutePath();

        // Change to the game file's directory.
        QDir::setCurrent(finfo.absolutePath());

        // Set the application's window title to contain the filename of the game we're running. The
        // game is free to change that later on.
#ifdef Q_OS_MAC
        // Just use the filename on OS X.  Seems to be the norm there.
        // qWinGroup->setWindowTitle(finfo.fileName());
#else
        // On all other systems, also append the application name.
        // qWinGroup->setWindowTitle(finfo.fileName() + QString::fromLatin1(" - ") +
        // qFrame->applicationName());
#endif

        // Add the game file to our "recent games" list.
        QStringList& gamesList = settings_.recent_games_list;
        int recentIdx = gamesList.indexOf(finfo.absoluteFilePath());
        if (recentIdx > 0) {
            // It's already in the list and it's not the first item. Make it the first item so that
            // it becomes the most recent entry.
            gamesList.move(recentIdx, 0);
        } else if (recentIdx < 0) {
            // We didn't find it in the list by absoluteFilePath(). Try to find it by
            // canonicalFilePath() instead. This way, we avoid listing the same game twice if the
            // user opened it through a different path (through a symlink that leads to the same
            // file, for instance.)
            bool found = false;
            const QString& canonPath = finfo.canonicalFilePath();
            for (recentIdx = 0; recentIdx < gamesList.size() and not found; ++recentIdx) {
                if (QFileInfo(gamesList.at(recentIdx)).canonicalFilePath() == canonPath) {
                    found = true;
                }
            }
            if (found) {
                gamesList.move(recentIdx - 1, 0);
            } else {
                // It's not in the list. Prepend it as the most recent item and, if the list is
                // full, delete the oldest one.
                if (gamesList.size() >= Settings::recent_games_capacity) {
                    gamesList.removeLast();
                }
                gamesList.prepend(finfo.absoluteFilePath());
            }
        }
        settings_.saveToDisk();

        // Run the Hugo engine.
        is_game_running_ = true;
        gamefile_ = finfo.absoluteFilePath();
        main_win_->setUpdatesEnabled(true);
        main_win_->raise();
        main_win_->activateWindow();
        hugo_thread_ = new EngineThread(this);
        hugo_thread_->setObjectName("engine");
        engine_runner_ = new EngineRunner(gamefile_, hugo_thread_);
        engine_runner_->moveToThread(hugo_thread_);
        connect(engine_runner_, &EngineRunner::finished, hugo_thread_, &EngineThread::quit);
        connect(hugo_thread_, &QThread::started, engine_runner_, &EngineRunner::runEngine);
        connect(hugo_thread_, &QThread::finished, engine_runner_, &EngineRunner::deleteLater);
        connect(hugo_thread_, &QThread::finished, hugo_thread_, &EngineThread::deleteLater);
        connect(hugo_thread_, &QThread::finished, this, &HApplication::handleEngineFinished);
        hugo_thread_->start();
    }
}

void HApplication::updateMarginColor(int color)
{
    if (color < 0) {
        return;
    }

    const Settings& sett = hApp->settings();
    const QColor& qColor = (hMainWin->isFullScreen() and sett.custom_fs_margin_color)
                               ? sett.fs_margin_color
                               : hugoColorToQt(color);
    margin_widget_->setColor(qColor);
}

void HApplication::updateMargins(int color)
{
    int margin;

    // In fullscreen mode, respect the aspect ratio and max width settings.
    if (hMainWin->isFullScreen()) {
        int scrWidth = QApplication::primaryScreen()->size().width();
        int maxWidth = settings_.fullscreen_width * scrWidth / 100;

        // Calculate how big the margin should be to get the specified width.
        int targetWidth = qMin(maxWidth, margin_widget_->width());
        margin = (margin_widget_->width() - targetWidth) / 2;
    } else {
        // In windowed mode, do not update the margins if we're currently displaying scrollback as
        // an overlay.
        if (margin_widget_->layout()->indexOf(frame_win_) < 0) {
            return;
        }
        margin = settings_.margin_size;
    }
    margin_widget_->setContentsMargins(margin, 0, margin, bottom_margin_size_);
    updateMarginColor(color);
}

#ifdef Q_OS_MAC
#include <QFileOpenEvent>
bool HApplication::event(QEvent* e)
{
    // We only handle the FileOpen event and only when no game
    // is currently running.
    if (e->type() != QEvent::FileOpen or is_game_running_) {
        return QApplication::event(e);
    }
    QFileOpenEvent* fOpenEv = static_cast<QFileOpenEvent*>(e);
    if (fOpenEv->file().isEmpty()) {
        return QApplication::event(e);
    }
    next_game_ = fOpenEv->file();
    e->accept();
    return true;
}
#endif

void HApplication::entryPoint(QString gameFileName)
{
    // Process pending events in case we have a FileOpen event. Freeze user input while doing so; we
    // don't want to leave a way to mess with the GUI when we don't have a game running yet. We do
    // this a bunch of times to make sure the FileOpen event can propagate properly.
    for (int i = 0; i < 100; ++i) {
        advanceEventLoop();
    }

    if (next_game_.isEmpty()) {
        next_game_ = std::move(gameFileName);
    }

    // If we still don't have a filename, prompt for one.
    if (next_game_.isEmpty() and settings_.ask_for_gamefile) {
        next_game_ = QFileDialog::getOpenFileName(
            nullptr, QObject::tr("Choose the story file you wish to play"),
            settings_.last_file_open_dir,
            QObject::tr("Hugo Games") + QString::fromLatin1("(*.hex)"));
    }

    // Switch to fullscreen, if needed.
    if (settings_.is_fullscreen) {
        main_win_->toggleFullscreen();
        // If we don't let the event loop run for a while, for some reason the screen will be
        // flashing when the window first becomes visible. The reason is unknown, but this seems to
        // work around the issue.
        for (int i = 0; i < 5000; ++i) {
            advanceEventLoop();
        }
    }

    // Automatically quit the application when the last window has closed.
    connect(this, &HApplication::lastWindowClosed, this, &HApplication::quit);

    // If we have a filename, load it.
    if (not next_game_.isEmpty()) {
        if (settings_.is_maximized) {
            main_win_->showMaximized();
        } else {
            main_win_->show();
        }
        runGame();
    } else {
        // File dialog was canceled.
        quit();
    }
}

void HApplication::handleEngineFinished()
{
    is_game_running_ = false;
    gamefile_.clear();
    emit gameHasQuit();
    main_win_->close();
}

void HApplication::notifyPreferencesChange(const Settings& sett)
{
    smartformatting = sett.smart_formatting;

    // 'bgcolor' is a Hugo engine global.
    updateMargins(::bgcolor);

    // Recalculate font dimensions, in case font settings have changed.
    HugoHandlers::calcFontDimensions();

    // The fonts might have changed.
    hFrame->setFontType(currentfont);
    hMainWin->setScrollbackFont(sett.scrollback_font);

    display_needs_repaint = true;
#ifndef DISABLE_AUDIO
    if (not sett.enable_music) {
        HugoHandlers::stopmusic();
    }
    if (not sett.enable_sound_effects) {
        HugoHandlers::stopsample();
    }
    updateMusicVolume();
    updateSoundVolume();
    updateVideoVolume();
#endif
#ifndef DISABLE_VIDEO
    if (not sett.enable_video) {
        HugoHandlers::stopvideo();
    }
#endif
    frame_win_->updateGameScreen(true);
}

void HApplication::advanceEventLoop()
{
    // Guard against re-entrancy.
    static volatile bool working = false;
    if (working) {
        return;
    }
    working = true;
    HApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    working = false;
}

void HApplication::terminateEngineThread()
{
    // FIXME This just doesn't work reliably. On Windows it just hangs.
    /*
    fHugoThread->terminate();
    fHugoThread->wait(2000);
    */
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
