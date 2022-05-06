// This is copyrighted software. More information is at the end of this file.
#include "settingsoverrides.h"

#include <QSettings>

SettingsOverrides::SettingsOverrides(const QString& filename)
{
    QSettings sett(filename, QSettings::IniFormat);
    sett.setIniCodec("UTF-8");

    // Note: unquoted ini values with commas are treated as QStringList by Qt. So we need treat all
    // string values as string lists and join them into a single string with a comma as seperator.

    const auto& all_keys = sett.allKeys();
    for (const auto& original_key : all_keys) {
        const auto& key = original_key.toLower();
        if (key == "identity/appname") {
            app_name = sett.value(original_key).toStringList().join(',');
        } else if (key == "identity/authorname") {
            author_name = sett.value(original_key).toStringList().join(',');
        } else if (key == "display/fullscreen") {
            fullscreen = sett.value(original_key).toBool();
        } else if (key == "display/fullscreenwidth") {
            fullscreen_width = sett.value(original_key).toInt();
            if (fullscreen_width > 0) {
                if (fullscreen_width > 100) {
                    fullscreen_width = 100;
                } else if (fullscreen_width < 10) {
                    fullscreen_width = 10;
                }
            }
        } else if (key == "display/hidemenubar") {
            hide_menubar = sett.value(original_key).toBool();
        } else if (key == "display/marginsize") {
            margin_size = sett.value(original_key).toInt();
        } else if (key == "display/widthratio") {
            width_ratio = sett.value(original_key).toInt();
        } else if (key == "display/heightratio") {
            height_ratio = sett.value(original_key).toInt();
        } else if (key == "display/propfont") {
            prop_font = sett.value(original_key).toStringList().join(',');
        } else if (key == "display/fixedfont") {
            fixed_font = sett.value(original_key).toStringList().join(',');
        } else if (key == "display/scrollbackfont") {
            scrollback_font = sett.value(original_key).toStringList().join(',');
        } else if (key == "display/propfontsize") {
            prop_font_size = sett.value(original_key).toInt();
        } else if (key == "display/fixedfontsize") {
            fixed_font_size = sett.value(original_key).toInt();
        } else if (key == "display/scrollbackfontsize") {
            scrollback_font_size = sett.value(original_key).toInt();
        } else if (key == "display/fsmargincolor") {
            QString namedColor = sett.value(original_key).toStringList().join(',');
            fs_margin_color.setNamedColor(namedColor);
        } else if (key == "media/mutewhenminimized") {
            mute_when_minimized = sett.value(original_key).toBool();
        } else if (key == "bundledfonts/fontpath") {
            font_dir = sett.value(original_key).toStringList().join(',');
        }
    }
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
