#include "pdf_exporter.h"
#include "theme_decors.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QGraphicsScene>

bool PdfExporter::exportProjectToPdf(
    const BookProject &project,
    const BookTheme &theme,
    QGraphicsScene *currentScene,
    const QString &currentSpreadId,
    const QString &outputPath
) {
    QPdfWriter pdfWriter(outputPath);
    pdfWriter.setResolution(300);

    QPageLayout layout(
        QPageSize(QSizeF(1200, 650), QPageSize::Point),
        QPageLayout::Landscape,
        QMarginsF(0, 0, 0, 0)
    );
    pdfWriter.setPageLayout(layout);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRectF pageRect(0, 0, 1200, 650);

    for (int i = 0; i < project.spreads.size(); ++i) {
        if (i > 0) {
            pdfWriter.newPage();
        }

        const auto &spread = project.spreads[i];

        painter.fillRect(pageRect, theme.paperBg);

        QRectF leftPage(0, 0, 600, 650);
        QRectF rightPage(600, 0, 600, 650);

        painter.setPen(QPen(theme.borderColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(leftPage.adjusted(12, 12, -12, -12));
        painter.drawRect(rightPage.adjusted(12, 12, -12, -12));

        ThemeDecors::draw(&painter, theme);

        painter.setPen(QPen(QColor(0, 0, 0, 100), 1));
        painter.drawLine(600, 0, 600, 650);

        if (spread.id == currentSpreadId && currentScene) {
            currentScene->render(&painter, pageRect, pageRect);
        }
    }

    painter.end();
    return true;
}