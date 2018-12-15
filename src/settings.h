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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QFont>
#include <QColor>
#include <QSize>


class Settings {
  public:
    Settings()
        : videoSysError(false)
    { }

    void
    loadFromDisk( class SettingsOverrides* ovr = 0 );

    void
    saveToDisk();

    bool enableGraphics;
    bool enableVideo;
    bool enableSoundEffects;
    bool enableMusic;
    bool useSmoothScaling;
    bool muteWhenMinimized;
    int soundVolume;
    bool useCustomSoundFont;
    QString soundFont;
    float synthGain;

    QColor mainTextColor;
    QColor mainBgColor;
    QColor statusTextColor;
    QColor statusBgColor;
    bool customFsMarginColor;
    QColor fsMarginColor;

    QFont propFont;
    QFont fixedFont;
    QFont scrollbackFont;
    bool softTextScrolling;
    bool smartFormatting;
    int scriptWrap;

    bool askForGameFile;
    QString lastFileOpenDir;

    QStringList recentGamesList;
    static const int recentGamesCapacity = 10;

    QSize appSize;
    bool isMaximized;
    bool isFullscreen;
    bool overlayScrollback;
    int marginSize;
    int fullscreenWidth;
    bool startFullscreen;
    bool startWindowed;

    // These are not saved. Used for temporary overrides that only apply
    // to the current session.
    bool videoSysError;
};


#endif
