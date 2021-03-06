// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <QColor>
#include <QString>

class SettingsOverrides final
{
public:
    SettingsOverrides(const QString& filename);

    QString app_name;
    QString author_name;
    bool fullscreen;
    int fullscreen_width;
    bool hide_menubar;
    int margin_size;
    QColor fs_margin_color;
    int width_ratio;
    int height_ratio;
    int prop_font_size;
    int fixed_font_size;
    int scrollback_font_size;
    bool mute_when_minimized;
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
 */
