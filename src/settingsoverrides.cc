// This is copyrighted software. More information is at the end of this file.
#include "settingsoverrides.h"

#include <QSettings>

SettingsOverrides::SettingsOverrides(const QString& filename)
{
    QSettings sett(filename, QSettings::IniFormat);

    sett.beginGroup(QString::fromLatin1("identity"));
    app_name = sett.value(QString::fromLatin1("appName"), QString::fromLatin1("")).toString();
    author_name = sett.value(QString::fromLatin1("authorName"), QString::fromLatin1("")).toString();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("display"));
    fullscreen = sett.value(QString::fromLatin1("fullscreen"), false).toBool();
    hide_menubar = sett.value(QString::fromLatin1("hideMenuBar"), false).toBool();
    fullscreen_width = sett.value(QString::fromLatin1("fullscreenWidth"), 0).toInt();
    if (fullscreen_width > 0) {
        if (fullscreen_width > 100) {
            fullscreen_width = 100;
        } else if (fullscreen_width < 10) {
            fullscreen_width = 10;
        }
    }
    margin_size = sett.value(QString::fromLatin1("marginSize"), 0).toInt();
    width_ratio = sett.value(QString::fromLatin1("widthRatio"), 4).toInt();
    height_ratio = sett.value(QString::fromLatin1("heightRatio"), 3).toInt();
    prop_font_size = sett.value(QString::fromLatin1("propFontSize"), 0).toInt();
    fixed_font_size = sett.value(QString::fromLatin1("fixedFontSize"), 0).toInt();
    scrollback_font_size = sett.value(QString::fromLatin1("scrollbackFontSize"), 0).toInt();
    image_smoothing = sett.value(QString::fromLatin1("imageSmoothing"), true).toBool();
    QString namedColor = sett.value(QString::fromLatin1("fsMarginColor"), QString()).toString();
    fs_margin_color.setNamedColor(namedColor);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("media"));
    mute_when_minimized = sett.value(QString::fromLatin1("muteWhenMinimized"), true).toBool();
    sett.endGroup();
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
