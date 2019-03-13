// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QColor>
#include <QFont>
#include <QSize>

class SettingsOverrides;

class Settings final
{
    Q_GADGET

public:
    enum class TextCursorShape
    {
        Ibeam,
        Block,
        Underline,
    };
    Q_ENUM(TextCursorShape)

    Settings()
        : video_sys_error(false)
    {}

    void loadFromDisk(SettingsOverrides* ovr = nullptr);

    void saveToDisk();

    bool enable_graphics;
    bool enable_video;
    bool enable_sound_effects;
    bool enable_music;
    bool mute_when_minimized;
    int sound_volume;
    bool use_custom_soundfont;
    QString soundfont;
    float synth_gain;
    bool use_adlmidi;

    QColor main_text_color;
    QColor main_bg_color;
    QColor status_text_color;
    QColor status_bg_color;
    bool custom_fs_margin_color;
    QColor fs_margin_color;

    QFont prop_font;
    QFont fixed_font;
    QFont scrollback_font;
    bool soft_text_scrolling;
    bool smart_formatting;
    int script_wrap;
    TextCursorShape cursor_shape;
    int cursor_thickness;

    bool ask_for_gamefile;
    QString last_file_open_dir;

    QStringList recent_games_list;
    static const int recent_games_capacity = 10;

    QSize app_size;
    bool is_maximized;
    bool is_fullscreen;
    bool overlay_scrollback;
    bool scrollback_on_wheel;
    int margin_size;
    int fullscreen_width;
    bool start_fullscreen;
    bool start_windowed;

    // These are not saved. Used for temporary overrides that only apply to the current session.
    bool video_sys_error;
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
