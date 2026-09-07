#include "theme_decors.h"
#include <QSvgRenderer>

QString ThemeDecors::getCornerSvg(const BookTheme &theme, const QString &placement) {
    QString color = theme.frameColor.name();
    QString accent = theme.accentColor.name();
    QString paper = theme.paperBg.name();

    if (theme.decorType == DecorType::Botanical) {
        return QString(R"(
            <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
              <path d="M 5 65 Q 10 20 65 5 Q 35 15 25 35 Q 15 45 5 65 Z" fill="%1" fill-opacity="0.8" />
              <path d="M 12 40 Q 22 24 40 22 Q 26 34 12 40 Z" fill="%2" />
              <circle cx="18" cy="20" r="3" fill="%2" />
              <circle cx="28" cy="14" r="2.5" fill="%1" />
              <circle cx="44" cy="10" r="3" fill="%2" />
            </svg>
        )").arg(color, accent);
    }

    if (theme.decorType == DecorType::Celestial) {
        return QString(R"(
            <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
              <path d="M 8 8 L 8 40 M 8 8 L 40 8" stroke="%1" stroke-width="2" fill="none" />
              <path d="M 22 10 A 10 10 0 0 0 10 22 A 10 10 0 1 1 22 10 Z" fill="%2" />
              <polygon points="32,16 35,22 41,25 35,28 32,34 29,28 23,25 29,22" fill="%1" />
              <circle cx="16" cy="36" r="2" fill="%2" />
              <circle cx="40" cy="12" r="1.5" fill="%1" />
            </svg>
        )").arg(color, accent);
    }

    if (theme.decorType == DecorType::Victorian) {
        return QString(R"(
            <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
              <path d="M 6 6 L 6 45 Q 6 18 35 18 L 45 18 M 6 6 L 45 6 Q 18 6 18 35 L 18 45" stroke="%1" stroke-width="2" fill="none" />
              <path d="M 14 14 Q 28 14 32 32 Q 14 28 14 14 Z" fill="%2" fill-opacity="0.7" />
              <circle cx="6" cy="6" r="4" fill="%1" />
            </svg>
        )").arg(color, accent);
    }

    if (theme.decorType == DecorType::Sakura) {
        return QString(R"(
            <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
              <path d="M 4 60 Q 20 40 35 35 Q 50 30 65 4" stroke="%1" stroke-width="2" fill="none" />
              <circle cx="28" cy="24" r="5" fill="%2" />
              <circle cx="40" cy="18" r="4" fill="%1" fill-opacity="0.8" />
              <circle cx="16" cy="40" r="4.5" fill="%2" fill-opacity="0.9" />
              <circle cx="48" cy="30" r="3" fill="%1" />
            </svg>
        )").arg(color, accent);
    }

    if (theme.decorType == DecorType::Chai) {
        return QString(R"(
            <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
              <line x1="8" y1="8" x2="8" y2="48" stroke="%1" stroke-width="3" stroke-dasharray="5,4" />
              <line x1="8" y1="8" x2="48" y2="8" stroke="%1" stroke-width="3" stroke-dasharray="5,4" />
              <rect x="14" y="14" width="14" height="14" fill="%2" transform="rotate(45 21 21)" />
            </svg>
        )").arg(color, accent);
    }

    return QString(R"(
        <svg viewBox="0 0 70 70" xmlns="http://www.w3.org/2000/svg">
          <polygon points="4,4 50,4 32,22 22,22 22,32 4,50" fill="%1" />
          <circle cx="14" cy="14" r="3" fill="%2" />
          <circle cx="34" cy="10" r="2" fill="%2" />
          <circle cx="10" cy="34" r="2" fill="%2" />
        </svg>
    )").arg(color, paper);
}

void ThemeDecors::draw(QPainter *painter, const BookTheme &theme) {
    auto renderCorner = [&](const QString &placement, qreal centerX, qreal centerY, bool flipX, bool flipY) {
        QString svg = getCornerSvg(theme, placement);
        QSvgRenderer renderer(svg.toUtf8());

        painter->save();
        painter->translate(centerX, centerY);
        painter->scale(flipX ? -1.0 : 1.0, flipY ? -1.0 : 1.0);
        renderer.render(painter, QRectF(-35, -35, 70, 70));
        painter->restore();
    };

    renderCorner("tl", 49, 49, false, false);
    renderCorner("bl", 49, 601, false, true);

    renderCorner("tr", 1151, 49, true, false);
    renderCorner("br", 1151, 601, true, true);
}