#pragma once

#include <QString>
#include "types.h"

class PdfExporter {
public:
    enum class Layout {
        Spreads,     // one PDF page per two-page spread, as seen on screen
        SinglePages  // one PDF page per book page, for printing
    };

    // Renders every spread from its saved canvas data, so the project must be committed first.
    static bool exportProjectToPdf(
        const BookProject &project,
        const BookTheme &theme,
        Layout layout,
        const QString &outputPath
    );
};
