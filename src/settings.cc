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
#include "settings.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QSettings>

extern "C" {
#include "heheader.h"
}
#include "hmainwindow.h"
#include "hugodefs.h"
#include "settingsoverrides.h"

#define SETT_MEDIA_GRP QString::fromLatin1("media")
#define SETT_COLORS_GRP QString::fromLatin1("colors")
#define SETT_FONTS_GRP QString::fromLatin1("fonts")
#define SETT_RECENT_GRP QString::fromLatin1("recent")
#define SETT_MISC_GRP QString::fromLatin1("misc")
#define SETT_GEOMETRY_GRP QString::fromLatin1("geometry")
#define SETT_GRAPHICS QString::fromLatin1("graphics")
#define SETT_VIDEO QString::fromLatin1("video")
#define SETT_SOUNDS QString::fromLatin1("sounds")
#define SETT_MUSIC QString::fromLatin1("music")
#define SETT_USE_CUSTOM_SOUNDFONT QString::fromLatin1("usecustomsoundfont")
#define SETT_SOUNDFONT QString::fromLatin1("soundfont")
#define SETT_SYNTH_GAIN QString::fromLatin1("synthgain")
#define SETT_SMOOTH_IMAGES QString::fromLatin1("smoothImageScaling")
#define SETT_MUTE_MINIMIZED QString::fromLatin1("muteWhenMinimized")
#define SETT_SOUND_VOL QString::fromLatin1("soundVolume")
#define SETT_MAIN_BG_COLOR QString::fromLatin1("mainbg")
#define SETT_MAIN_TXT_COLOR QString::fromLatin1("maintext")
#define SETT_STATUS_BG_COLOR QString::fromLatin1("bannerbg")
#define SETT_STATUS_TXT_COLOR QString::fromLatin1("bannertext")
#define SETT_CUSTOM_FS_MARGIN_COLOR QString::fromLatin1("customFsMarginColor")
#define SETT_MARGIN_COLOR QString::fromLatin1("fullscreenMargin")
#define SETT_MAIN_FONT QString::fromLatin1("main")
#define SETT_FIXED_FONT QString::fromLatin1("fixed")
#define SETT_SCROLLBACK_FONT QString::fromLatin1("scrollback")
#define SETT_SOFT_SCROLL QString::fromLatin1("softTextScrolling")
#define SETT_SMART_FORMATTING QString::fromLatin1("smartFormatting")
#define SETT_SCRIPT_WRAP QString::fromLatin1("scriptWrap")
#define SETT_ASK_FILE QString::fromLatin1("askforfileatstart")
#define SETT_LAST_OPEN_DIR QString::fromLatin1("lastFileOpenDir")
#define SETT_GAMES_LIST QString::fromLatin1("games")
#define SETT_APP_SIZE QString::fromLatin1("size")
#define SETT_OVERLAY_SCROLL QString::fromLatin1("overlayScrollback")
#define SETT_MAXIMIZED QString::fromLatin1("maximized")
#define SETT_FULLSCREEN QString::fromLatin1("fullscreen")
#define SETT_MARGIN_SIZE QString::fromLatin1("marginSize")
#define SETT_FULLSCREEN_WIDTH QString::fromLatin1("fullscreenWidth")
#define SETT_START_FULLSCREEN QString::fromLatin1("startFullscreen")
#define SETT_START_WINDOWED QString::fromLatin1("startWindowed")

void Settings::loadFromDisk(SettingsOverrides* ovr)
{
    QSettings sett;

    sett.beginGroup(SETT_MEDIA_GRP);
    enableGraphics = sett.value(SETT_GRAPHICS, true).toBool();
    enableVideo = sett.value(SETT_VIDEO, true).toBool();
#ifndef Q_OS_ANDROID
    enableSoundEffects = sett.value(SETT_SOUNDS, true).toBool();
    enableMusic = sett.value(SETT_MUSIC, true).toBool();
#else
    enableSoundEffects = sett.value(SETT_SOUNDS, false).toBool();
    enableMusic = sett.value(SETT_MUSIC, false).toBool();
#endif
    useSmoothScaling = sett.value(SETT_SMOOTH_IMAGES, true).toBool();
    muteWhenMinimized = sett.value(SETT_MUTE_MINIMIZED, true).toBool();
    soundVolume = sett.value(SETT_SOUND_VOL, 100).toInt();
    useCustomSoundFont = sett.value(SETT_USE_CUSTOM_SOUNDFONT, false).toBool();
    soundFont = sett.value(SETT_SOUNDFONT, QString()).toString();
    synthGain = sett.value(SETT_SYNTH_GAIN, 0.6f).toFloat();
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    mainBgColor = sett.value(SETT_MAIN_BG_COLOR, hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    mainTextColor = sett.value(SETT_MAIN_TXT_COLOR, hugoColorToQt(DEF_FCOLOR)).value<QColor>();
    statusBgColor = sett.value(SETT_STATUS_BG_COLOR, hugoColorToQt(DEF_SLBGCOLOR)).value<QColor>();
    statusTextColor =
        sett.value(SETT_STATUS_TXT_COLOR, hugoColorToQt(DEF_SLFCOLOR)).value<QColor>();
    customFsMarginColor = sett.value(SETT_CUSTOM_FS_MARGIN_COLOR, false).toBool();
    fsMarginColor = sett.value(SETT_MARGIN_COLOR, hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    sett.endGroup();

#ifdef Q_OS_MAC
    const QString& DEFAULT_PROP = QString::fromLatin1("Georgia,15");
    const QString& DEFAULT_MONO = QString::fromLatin1("Andale Mono,15");
#else
#ifdef Q_OS_WIN
    const QString& DEFAULT_PROP = QString::fromLatin1("Times New Roman,12");
    const QString& DEFAULT_MONO = QString::fromLatin1("Courier New,12");
#else
#ifdef Q_OS_ANDROID
    const QString& DEFAULT_PROP = QString::fromLatin1("Droid Serif");
    const QString& DEFAULT_MONO = QString::fromLatin1("Droid Sans Mono");
#else
    const QString& DEFAULT_PROP = QString::fromLatin1("serif,12");
    const QString& DEFAULT_MONO = QString::fromLatin1("monospace,12");
#endif
#endif
#endif
    sett.beginGroup(SETT_FONTS_GRP);
    QFont::StyleStrategy strat;
    strat = QFont::StyleStrategy(QFont::PreferOutline | QFont::PreferQuality
                                 | QFont::ForceIntegerMetrics);
    propFont.setStyleStrategy(strat);
    QFont tmp;
    tmp.fromString(sett.value(SETT_MAIN_FONT, DEFAULT_PROP).toString());
    propFont.setFamily(tmp.family());
    propFont.setPointSize(tmp.pointSize());
    fixedFont.setStyleStrategy(strat);
    tmp.fromString(sett.value(SETT_FIXED_FONT, DEFAULT_MONO).toString());
    fixedFont.setFamily(tmp.family());
    fixedFont.setPointSize(tmp.pointSize());
    scrollbackFont.setStyleStrategy(strat);
    tmp.fromString(sett.value(SETT_SCROLLBACK_FONT, DEFAULT_PROP).toString());
    scrollbackFont.setFamily(tmp.family());
    scrollbackFont.setPointSize(tmp.pointSize());
    softTextScrolling = sett.value(SETT_SOFT_SCROLL, true).toBool();
    smartFormatting = sett.value(SETT_SMART_FORMATTING, true).toBool();
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    askForGameFile = sett.value(SETT_ASK_FILE, true).toBool();
    lastFileOpenDir = sett.value(SETT_LAST_OPEN_DIR, QString::fromLatin1("")).toString();
    scriptWrap = sett.value(SETT_SCRIPT_WRAP, 0).toInt();
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    recentGamesList = sett.value(SETT_GAMES_LIST, QStringList()).toStringList();
    Q_ASSERT(recentGamesList.size() <= recentGamesCapacity);
    // Remove any files that don't exist or aren't readable.
    for (int i = 0; i < recentGamesList.size(); ++i) {
        QFileInfo file(recentGamesList.at(i));
        if (not file.exists() or not(file.isFile() or file.isSymLink()) or not file.isReadable()) {
            recentGamesList.removeAt(i);
            --i;
        }
    }
    sett.endGroup();

    // If an aspect ratio is specified in the overrides, use it.
    int widthRatio;
    int heightRatio;
    if (ovr != nullptr) {
        widthRatio = ovr->widthRatio;
        heightRatio = ovr->heightRatio;
    } else {
        widthRatio = heightRatio = 1;
    }

    sett.beginGroup(SETT_GEOMETRY_GRP);
    appSize = sett.value(SETT_APP_SIZE, QSize(800, 600)).toSize();
    overlayScrollback = sett.value(SETT_OVERLAY_SCROLL, true).toBool();
    isMaximized = sett.value(SETT_MAXIMIZED, false).toBool();
    isFullscreen = sett.value(SETT_FULLSCREEN, true).toBool();
    marginSize = sett.value(SETT_MARGIN_SIZE, 0).toInt();
    // If fullscreen width is not set, use one that results in a 4:3 ratio.
    int scrHeight = QApplication::desktop()->screenGeometry().height();
    int scrWidth = QApplication::desktop()->screenGeometry().width();
    fullscreenWidth = sett.value(SETT_FULLSCREEN_WIDTH,
                                 (double)scrHeight * ((double)widthRatio / (double)heightRatio)
                                     * 100.0 / (double)scrWidth)
                          .toInt();
    if (fullscreenWidth > 100) {
        fullscreenWidth = 100;
    } else if (fullscreenWidth < 10) {
        fullscreenWidth = 10;
    }
    startFullscreen = sett.value(SETT_START_FULLSCREEN, true).toBool();
    if (startFullscreen) {
        isFullscreen = true;
    }
    startWindowed = sett.value(SETT_START_WINDOWED, false).toBool();
    if (startWindowed) {
        isFullscreen = false;
    }
    sett.endGroup();

    // Apply overrides for non-existent settings.
    if (ovr != nullptr) {
        sett.beginGroup(SETT_GEOMETRY_GRP);
        if (not sett.contains(SETT_FULLSCREEN)) {
            isFullscreen = ovr->fullscreen;
        }
        if (not sett.contains(SETT_FULLSCREEN_WIDTH) and ovr->fullscreenWidth > 0) {
            fullscreenWidth = ovr->fullscreenWidth;
        }
        if (not sett.contains(SETT_MARGIN_SIZE)) {
            marginSize = ovr->marginSize;
        }
        sett.endGroup();

        sett.beginGroup(SETT_FONTS_GRP);
        if (not sett.contains(SETT_MAIN_FONT) and ovr->propFontSize > 0) {
            propFont.setPointSize(ovr->propFontSize);
        }
        if (not sett.contains(SETT_FIXED_FONT) and ovr->fixedFontSize > 0) {
            fixedFont.setPointSize(ovr->fixedFontSize);
        }
        if (not sett.contains(SETT_SCROLLBACK_FONT) and ovr->scrollbackFontSize > 0) {
            scrollbackFont.setPointSize(ovr->scrollbackFontSize);
        }
        sett.endGroup();

        sett.beginGroup(SETT_MEDIA_GRP);
        if (not sett.contains(SETT_SMOOTH_IMAGES)) {
            useSmoothScaling = ovr->imageSmoothing;
        }
        if (not sett.contains(SETT_MUTE_MINIMIZED)) {
            muteWhenMinimized = ovr->muteWhenMinimized;
        }
        sett.endGroup();

        sett.beginGroup(SETT_COLORS_GRP);
        if (not sett.contains(SETT_CUSTOM_FS_MARGIN_COLOR) and ovr->fsMarginColor.isValid()) {
            fsMarginColor = ovr->fsMarginColor;
            customFsMarginColor = true;
        }
        sett.endGroup();
    }
}

void Settings::saveToDisk()
{
    QSettings sett;

    sett.beginGroup(SETT_MEDIA_GRP);
    sett.setValue(SETT_GRAPHICS, enableGraphics);
    sett.setValue(SETT_VIDEO, enableVideo);
    sett.setValue(SETT_SOUNDS, enableSoundEffects);
    sett.setValue(SETT_MUSIC, enableMusic);
    sett.setValue(SETT_SMOOTH_IMAGES, useSmoothScaling);
    sett.setValue(SETT_MUTE_MINIMIZED, muteWhenMinimized);
    sett.setValue(SETT_SOUND_VOL, soundVolume);
    sett.setValue(SETT_USE_CUSTOM_SOUNDFONT, useCustomSoundFont);
    sett.setValue(SETT_SOUNDFONT, soundFont);
    sett.setValue(SETT_SYNTH_GAIN, synthGain);
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    sett.setValue(SETT_MAIN_BG_COLOR, mainBgColor);
    sett.setValue(SETT_MAIN_TXT_COLOR, mainTextColor);
    sett.setValue(SETT_STATUS_BG_COLOR, statusBgColor);
    sett.setValue(SETT_STATUS_TXT_COLOR, statusTextColor);
    sett.setValue(SETT_CUSTOM_FS_MARGIN_COLOR, customFsMarginColor);
    sett.setValue(SETT_MARGIN_COLOR, fsMarginColor);
    sett.endGroup();

    sett.beginGroup(SETT_FONTS_GRP);
    sett.setValue(SETT_MAIN_FONT, propFont.toString());
    sett.setValue(SETT_FIXED_FONT, fixedFont.toString());
    sett.setValue(SETT_SCROLLBACK_FONT, scrollbackFont.toString());
    sett.setValue(SETT_SOFT_SCROLL, softTextScrolling);
    sett.setValue(SETT_SMART_FORMATTING, smartFormatting);
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    sett.setValue(SETT_ASK_FILE, askForGameFile);
    sett.setValue(SETT_LAST_OPEN_DIR, lastFileOpenDir);
    sett.setValue(SETT_SCRIPT_WRAP, scriptWrap);
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    sett.setValue(SETT_GAMES_LIST, recentGamesList);
    sett.endGroup();

    sett.beginGroup(SETT_GEOMETRY_GRP);
    // Do not save current application size if we're in fullscreen mode, since we need to restore
    // the windowed, non-fullscreen size next time we run.
    if (hMainWin->isFullScreen()) {
        sett.setValue(SETT_APP_SIZE, appSize);
    } else {
        sett.setValue(SETT_APP_SIZE, hMainWin->size());
    }
    sett.setValue(SETT_OVERLAY_SCROLL, overlayScrollback);
    sett.setValue(SETT_MAXIMIZED, hMainWin->isMaximized());
    sett.setValue(SETT_FULLSCREEN, hMainWin->isFullScreen());
    sett.setValue(SETT_MARGIN_SIZE, marginSize);
    sett.setValue(SETT_FULLSCREEN_WIDTH, fullscreenWidth);
    sett.setValue(SETT_START_FULLSCREEN, startFullscreen);
    sett.setValue(SETT_START_WINDOWED, startWindowed);
    sett.endGroup();
    sett.sync();
}
