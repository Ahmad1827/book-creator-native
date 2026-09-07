#pragma once

#include <QString>
#include "types.h"

class QGraphicsScene;

class PdfExporter {
public:
    static bool exportProjectToPdf(
        const BookProject &project,
        const BookTheme &theme,
        QGraphicsScene *currentScene,
        const QString &currentSpreadId,
        const QString &outputPath
    );
};