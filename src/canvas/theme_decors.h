#pragma once

#include <QPainter>
#include "types.h"

class ThemeDecors {
public:
    static void draw(QPainter *painter, const BookTheme &theme);

private:
    static QString getCornerSvg(const BookTheme &theme, const QString &placement);
};