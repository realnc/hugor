#include "extcolors.h"

#include <array>
#include <cstdint>
#include <QColor>
#include <QDebug>

struct RgbTriplet {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

static std::array<RgbTriplet, 155> colors;

QColor
getExtendedColor(int id)
{
    if (id < 100 or id > 254) {
        qWarning() << Q_FUNC_INFO << "Invalid color ID:" << id;
        return QColor();
    }
    const auto& rgb = colors[id - 100];
    return QColor::fromRgb(rgb.r, rgb.g, rgb.b, rgb.a);
}

void
setExtendedColor(int id, int r, int g, int b, int a)
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
