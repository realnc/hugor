// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QApplication>

class EngineRunner;
class EngineThread;
class HApplication;
class HFrame;
class HMainWindow;
class HMarginWidget;
class Settings;

extern HApplication* hApp;

class HApplication final: public QApplication
{
    Q_OBJECT

private:
    // Preferences (fonts, colors, etc.)
    Settings* settings_;

    // Main application window.
    HMainWindow* main_win_;

    // Frame widget containing all subwindows.
    HFrame* frame_win_;

    // Parent of fFrameWin, provides margins.
    HMarginWidget* margin_widget_;

    const int bottom_margin_size_ = 0;

    // Are we currently executing a game?
    bool is_game_running_ = false;

    // Filename of the game we're currently executing.
    QString gamefile_;

    // The game we should try to run after the current one ends.
    QString next_game_;

    // Text codec used by the Hugo engine.
    QTextCodec* hugo_codec_;

    // Are we running in Gnome?
    bool is_desktop_gnome = false;

    // Hugo engine runner and thread.
    EngineRunner* engine_runner_ = nullptr;
    EngineThread* hugo_thread_ = nullptr;

    // Run the game file contained in fNextGame.
    void runGame();

    void updateMarginColor(int color);

#ifdef Q_OS_MAC
protected:
    // On the Mac, dropping a file on our application icon will generate a FileOpen event, so we
    // override this to be able to handle it.
    bool event(QEvent*) override;
#endif

signals:
    // Emitted prior to quitting a game.  The game has not quit yet when this is emitted.
    void gameQuitting();

    // Emitted after quiting a game.  The game has already quit when this is emitted.
    void gameHasQuit();

public slots:
    // Replacement for main().  We need this so that we can start the Hugo engine after the
    // QApplication main event loop has started.
    void entryPoint(QString gameFileName);

    void handleEngineFinished();

public:
    HApplication(int& argc, char* argv[], const char* appName, const char* appVersion,
                 const char* orgName, const char* orgDomain);

    ~HApplication() override;

    /* Passing a negative value as 'color' will keep the current margin color.
     */
    void updateMargins(int color);

    Settings* settings() const
    {
        return settings_;
    }

    HFrame* frameWindow() const
    {
        return frame_win_;
    }

    HMarginWidget* marginWidget() const
    {
        return margin_widget_;
    }

    bool gameRunning() const
    {
        return is_game_running_;
    }

    const QString& gameFile() const
    {
        return gamefile_;
    }

    void setGameRunning(bool f)
    {
        is_game_running_ = f;
        if (not f) {
            emit gameQuitting();
        }
    }

    // Notify the application that preferences have changed.
    void notifyPreferencesChange(const Settings* sett);

    // Advance the event loop.
    void advanceEventLoop();

    // Text codec used by Hugo.
    QTextCodec* hugoCodec() const
    {
        return hugo_codec_;
    }

    bool desktopIsGnome() const
    {
        return is_desktop_gnome;
    }

    void terminateEngineThread();
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
