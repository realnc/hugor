// This is copyrighted software. More information is at the end of this file.
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
    enable_graphics = sett.value(SETT_GRAPHICS, true).toBool();
    enable_video = sett.value(SETT_VIDEO, true).toBool();
#ifndef Q_OS_ANDROID
    enable_sound_effects = sett.value(SETT_SOUNDS, true).toBool();
    enable_music = sett.value(SETT_MUSIC, true).toBool();
#else
    enableSoundEffects = sett.value(SETT_SOUNDS, false).toBool();
    enableMusic = sett.value(SETT_MUSIC, false).toBool();
#endif
    use_smooth_scaling = sett.value(SETT_SMOOTH_IMAGES, true).toBool();
    mute_when_minimized = sett.value(SETT_MUTE_MINIMIZED, true).toBool();
    sound_volume = sett.value(SETT_SOUND_VOL, 100).toInt();
    use_custom_soundfont = sett.value(SETT_USE_CUSTOM_SOUNDFONT, false).toBool();
    soundfont = sett.value(SETT_SOUNDFONT, QString()).toString();
    synth_gain = sett.value(SETT_SYNTH_GAIN, 0.6f).toFloat();
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    main_bg_color = sett.value(SETT_MAIN_BG_COLOR, hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
    main_text_color = sett.value(SETT_MAIN_TXT_COLOR, hugoColorToQt(DEF_FCOLOR)).value<QColor>();
    status_bg_color =
        sett.value(SETT_STATUS_BG_COLOR, hugoColorToQt(DEF_SLBGCOLOR)).value<QColor>();
    status_text_color =
        sett.value(SETT_STATUS_TXT_COLOR, hugoColorToQt(DEF_SLFCOLOR)).value<QColor>();
    custom_fs_margin_color = sett.value(SETT_CUSTOM_FS_MARGIN_COLOR, false).toBool();
    fs_margin_color = sett.value(SETT_MARGIN_COLOR, hugoColorToQt(DEF_BGCOLOR)).value<QColor>();
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
    prop_font.setStyleStrategy(strat);
    QFont tmp;
    tmp.fromString(sett.value(SETT_MAIN_FONT, DEFAULT_PROP).toString());
    prop_font.setFamily(tmp.family());
    prop_font.setPointSize(tmp.pointSize());
    fixed_font.setStyleStrategy(strat);
    tmp.fromString(sett.value(SETT_FIXED_FONT, DEFAULT_MONO).toString());
    fixed_font.setFamily(tmp.family());
    fixed_font.setPointSize(tmp.pointSize());
    scrollback_font.setStyleStrategy(strat);
    tmp.fromString(sett.value(SETT_SCROLLBACK_FONT, DEFAULT_PROP).toString());
    scrollback_font.setFamily(tmp.family());
    scrollback_font.setPointSize(tmp.pointSize());
    soft_text_scrolling = sett.value(SETT_SOFT_SCROLL, true).toBool();
    smart_formatting = sett.value(SETT_SMART_FORMATTING, true).toBool();
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    ask_for_gamefile = sett.value(SETT_ASK_FILE, true).toBool();
    last_file_open_dir = sett.value(SETT_LAST_OPEN_DIR, QString::fromLatin1("")).toString();
    script_wrap = sett.value(SETT_SCRIPT_WRAP, 0).toInt();
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    recent_games_list = sett.value(SETT_GAMES_LIST, QStringList()).toStringList();
    Q_ASSERT(recent_games_list.size() <= recent_games_capacity);
    // Remove any files that don't exist or aren't readable.
    for (int i = 0; i < recent_games_list.size(); ++i) {
        QFileInfo file(recent_games_list.at(i));
        if (not file.exists() or not(file.isFile() or file.isSymLink()) or not file.isReadable()) {
            recent_games_list.removeAt(i);
            --i;
        }
    }
    sett.endGroup();

    // If an aspect ratio is specified in the overrides, use it.
    int widthRatio;
    int heightRatio;
    if (ovr != nullptr) {
        widthRatio = ovr->width_ratio;
        heightRatio = ovr->height_ratio;
    } else {
        widthRatio = heightRatio = 1;
    }

    sett.beginGroup(SETT_GEOMETRY_GRP);
    app_size = sett.value(SETT_APP_SIZE, QSize(800, 600)).toSize();
    overlay_scrollback = sett.value(SETT_OVERLAY_SCROLL, true).toBool();
    is_maximized = sett.value(SETT_MAXIMIZED, false).toBool();
    is_fullscreen = sett.value(SETT_FULLSCREEN, true).toBool();
    margin_size = sett.value(SETT_MARGIN_SIZE, 0).toInt();
    // If fullscreen width is not set, use one that results in a 4:3 ratio.
    int scrHeight = QApplication::desktop()->screenGeometry().height();
    int scrWidth = QApplication::desktop()->screenGeometry().width();
    fullscreen_width = sett.value(SETT_FULLSCREEN_WIDTH,
                                  (double)scrHeight * ((double)widthRatio / (double)heightRatio)
                                      * 100.0 / (double)scrWidth)
                           .toInt();
    if (fullscreen_width > 100) {
        fullscreen_width = 100;
    } else if (fullscreen_width < 10) {
        fullscreen_width = 10;
    }
    start_fullscreen = sett.value(SETT_START_FULLSCREEN, false).toBool();
    if (start_fullscreen) {
        is_fullscreen = true;
    }
    start_windowed = sett.value(SETT_START_WINDOWED, false).toBool();
    if (start_windowed) {
        is_fullscreen = false;
    }
    sett.endGroup();

    // Apply overrides for non-existent settings.
    if (ovr != nullptr) {
        sett.beginGroup(SETT_GEOMETRY_GRP);
        if (not sett.contains(SETT_FULLSCREEN)) {
            is_fullscreen = ovr->fullscreen;
        }
        if (not sett.contains(SETT_FULLSCREEN_WIDTH) and ovr->fullscreen_width > 0) {
            fullscreen_width = ovr->fullscreen_width;
        }
        if (not sett.contains(SETT_MARGIN_SIZE)) {
            margin_size = ovr->margin_size;
        }
        sett.endGroup();

        sett.beginGroup(SETT_FONTS_GRP);
        if (not sett.contains(SETT_MAIN_FONT) and ovr->prop_font_size > 0) {
            prop_font.setPointSize(ovr->prop_font_size);
        }
        if (not sett.contains(SETT_FIXED_FONT) and ovr->fixed_font_size > 0) {
            fixed_font.setPointSize(ovr->fixed_font_size);
        }
        if (not sett.contains(SETT_SCROLLBACK_FONT) and ovr->scrollback_font_size > 0) {
            scrollback_font.setPointSize(ovr->scrollback_font_size);
        }
        sett.endGroup();

        sett.beginGroup(SETT_MEDIA_GRP);
        if (not sett.contains(SETT_SMOOTH_IMAGES)) {
            use_smooth_scaling = ovr->image_smoothing;
        }
        if (not sett.contains(SETT_MUTE_MINIMIZED)) {
            mute_when_minimized = ovr->mute_when_minimized;
        }
        sett.endGroup();

        sett.beginGroup(SETT_COLORS_GRP);
        if (not sett.contains(SETT_CUSTOM_FS_MARGIN_COLOR) and ovr->fs_margin_color.isValid()) {
            fs_margin_color = ovr->fs_margin_color;
            custom_fs_margin_color = true;
        }
        sett.endGroup();
    }
}

void Settings::saveToDisk()
{
    QSettings sett;

    sett.beginGroup(SETT_MEDIA_GRP);
    sett.setValue(SETT_GRAPHICS, enable_graphics);
    sett.setValue(SETT_VIDEO, enable_video);
    sett.setValue(SETT_SOUNDS, enable_sound_effects);
    sett.setValue(SETT_MUSIC, enable_music);
    sett.setValue(SETT_SMOOTH_IMAGES, use_smooth_scaling);
    sett.setValue(SETT_MUTE_MINIMIZED, mute_when_minimized);
    sett.setValue(SETT_SOUND_VOL, sound_volume);
    sett.setValue(SETT_USE_CUSTOM_SOUNDFONT, use_custom_soundfont);
    sett.setValue(SETT_SOUNDFONT, soundfont);
    sett.setValue(SETT_SYNTH_GAIN, synth_gain);
    sett.endGroup();

    sett.beginGroup(SETT_COLORS_GRP);
    sett.setValue(SETT_MAIN_BG_COLOR, main_bg_color);
    sett.setValue(SETT_MAIN_TXT_COLOR, main_text_color);
    sett.setValue(SETT_STATUS_BG_COLOR, status_bg_color);
    sett.setValue(SETT_STATUS_TXT_COLOR, status_text_color);
    sett.setValue(SETT_CUSTOM_FS_MARGIN_COLOR, custom_fs_margin_color);
    sett.setValue(SETT_MARGIN_COLOR, fs_margin_color);
    sett.endGroup();

    sett.beginGroup(SETT_FONTS_GRP);
    sett.setValue(SETT_MAIN_FONT, prop_font.toString());
    sett.setValue(SETT_FIXED_FONT, fixed_font.toString());
    sett.setValue(SETT_SCROLLBACK_FONT, scrollback_font.toString());
    sett.setValue(SETT_SOFT_SCROLL, soft_text_scrolling);
    sett.setValue(SETT_SMART_FORMATTING, smart_formatting);
    sett.endGroup();

    sett.beginGroup(SETT_MISC_GRP);
    sett.setValue(SETT_ASK_FILE, ask_for_gamefile);
    sett.setValue(SETT_LAST_OPEN_DIR, last_file_open_dir);
    sett.setValue(SETT_SCRIPT_WRAP, script_wrap);
    sett.endGroup();

    sett.beginGroup(SETT_RECENT_GRP);
    sett.setValue(SETT_GAMES_LIST, recent_games_list);
    sett.endGroup();

    sett.beginGroup(SETT_GEOMETRY_GRP);
    // Do not save current application size if we're in fullscreen mode, since we need to restore
    // the windowed, non-fullscreen size next time we run.
    if (hMainWin->isFullScreen()) {
        sett.setValue(SETT_APP_SIZE, app_size);
    } else {
        sett.setValue(SETT_APP_SIZE, hMainWin->size());
    }
    sett.setValue(SETT_OVERLAY_SCROLL, overlay_scrollback);
    sett.setValue(SETT_MAXIMIZED, hMainWin->isMaximized());
    sett.setValue(SETT_FULLSCREEN, hMainWin->isFullScreen());
    sett.setValue(SETT_MARGIN_SIZE, margin_size);
    sett.setValue(SETT_FULLSCREEN_WIDTH, fullscreen_width);
    sett.setValue(SETT_START_FULLSCREEN, start_fullscreen);
    sett.setValue(SETT_START_WINDOWED, start_windowed);
    sett.endGroup();
    sett.sync();
}

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
