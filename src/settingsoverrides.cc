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
#include "settingsoverrides.h"

#include <QSettings>

SettingsOverrides::SettingsOverrides(const QString& filename)
{
    QSettings sett(filename, QSettings::IniFormat);

    sett.beginGroup(QString::fromLatin1("identity"));
    appName = sett.value(QString::fromLatin1("appName"), QString::fromLatin1("")).toString();
    authorName = sett.value(QString::fromLatin1("authorName"), QString::fromLatin1("")).toString();
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("display"));
    fullscreen = sett.value(QString::fromLatin1("fullscreen"), false).toBool();
    hideMenuBar = sett.value(QString::fromLatin1("hideMenuBar"), false).toBool();
    fullscreenWidth = sett.value(QString::fromLatin1("fullscreenWidth"), 0).toInt();
    if (fullscreenWidth > 0) {
        if (fullscreenWidth > 100) {
            fullscreenWidth = 100;
        } else if (fullscreenWidth < 10) {
            fullscreenWidth = 10;
        }
    }
    marginSize = sett.value(QString::fromLatin1("marginSize"), 0).toInt();
    widthRatio = sett.value(QString::fromLatin1("widthRatio"), 4).toInt();
    heightRatio = sett.value(QString::fromLatin1("heightRatio"), 3).toInt();
    propFontSize = sett.value(QString::fromLatin1("propFontSize"), 0).toInt();
    fixedFontSize = sett.value(QString::fromLatin1("fixedFontSize"), 0).toInt();
    scrollbackFontSize = sett.value(QString::fromLatin1("scrollbackFontSize"), 0).toInt();
    imageSmoothing = sett.value(QString::fromLatin1("imageSmoothing"), true).toBool();
    QString namedColor = sett.value(QString::fromLatin1("fsMarginColor"), QString()).toString();
    fsMarginColor.setNamedColor(namedColor);
    sett.endGroup();

    sett.beginGroup(QString::fromLatin1("media"));
    muteWhenMinimized = sett.value(QString::fromLatin1("muteWhenMinimized"), true).toBool();
    sett.endGroup();
}
