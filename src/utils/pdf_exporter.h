#pragma once

#include <QString>
#include <QStringList>
#include <QSizeF>
#include "types.h"

class PdfExporter {
public:
    enum class Layout {
        Spreads,     // one PDF page per two-page spread, as seen on screen
        SinglePages  // one PDF page per book page, for printing
    };

    struct Options {
        Layout layout = Layout::SinglePages;
        // Extends each page 1/8 inch past the trim on its outer edges, as printers require.
        bool bleed = false;
    };

    // Renders every spread from its saved canvas data, so the project must be committed first.
    static bool exportProjectToPdf(
        const BookProject &project,
        const BookTheme &theme,
        const Options &options,
        const QString &outputPath
    );

    // Pictures that will print below `minDpi` at the book's trim size, described for a warning.
    static QStringList lowResolutionPictures(const BookProject &project, qreal minDpi = 150.0);

    // The PDF page size in inches for these options.
    static QSizeF pageSizeInches(const BookProject &project, const Options &options);
};
