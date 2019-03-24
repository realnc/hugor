// This is copyrighted software. More information is at the end of this file.
#include "extcolors.h"

#include <QColor>
#include <QDebug>
#include <array>
#include <cstdint>

struct RgbTriplet
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

static std::array<RgbTriplet, 155> colors;

QColor getExtendedColor(int id)
{
    if (id < 100 or id > 254) {
        qWarning() << Q_FUNC_INFO << "Invalid color ID:" << id;
        return {};
    }
    const auto& rgb = colors[id - 100];
    return QColor::fromRgb(rgb.r, rgb.g, rgb.b, rgb.a);
}

void setExtendedColor(int id, int r, int g, int b, int a)
{
    if (id < 100 or id > 254) {
        qWarning() << Q_FUNC_INFO << "Invalid color ID:" << id;
        return;
    }
    auto& rgb = colors[id - 100];
    rgb.r = r;
    rgb.g = g;
    rgb.b = b;
    rgb.a = a;
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
