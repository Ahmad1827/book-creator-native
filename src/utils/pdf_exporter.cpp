#include "pdf_exporter.h"
#include "theme_decors.h"
#include "canvas_items.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QGraphicsScene>

namespace {

constexpr qreal kPageWidth = 600.0;
constexpr qreal kPageHeight = 650.0;

void paintSpreadPaper(QPainter &painter, const BookTheme &theme, bool drawGutter) {
    const QRectF spreadRect(0, 0, kPageWidth * 2, kPageHeight);
    const QRectF leftPage(0, 0, kPageWidth, kPageHeight);
    const QRectF rightPage(kPageWidth, 0, kPageWidth, kPageHeight);

    painter.fillRect(spreadRect, theme.paperBg);

    painter.setPen(QPen(theme.borderColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(leftPage.adjusted(12, 12, -12, -12));
    painter.drawRect(rightPage.adjusted(12, 12, -12, -12));

    ThemeDecors::draw(&painter, theme);

    if (drawGutter) {
        painter.setPen(QPen(QColor(0, 0, 0, 100), 1));
        painter.drawLine(QPointF(kPageWidth, 0), QPointF(kPageWidth, kPageHeight));
    }
}

} // namespace

bool PdfExporter::exportProjectToPdf(
    const BookProject &project,
    const BookTheme &theme,
    Layout layout,
    const QString &outputPath
) {
    if (project.spreads.isEmpty()) return false;

    const bool singlePages = layout == Layout::SinglePages;
    const QSizeF pageSize(singlePages ? kPageWidth : kPageWidth * 2, kPageHeight);

    QPdfWriter pdfWriter(outputPath);
    pdfWriter.setResolution(300);
    pdfWriter.setTitle(project.title);
    pdfWriter.setCreator(project.author);
    pdfWriter.setPageLayout(QPageLayout(
        QPageSize(pageSize, QPageSize::Point, QString(), QPageSize::ExactMatch),
        QPageLayout::Portrait,
        QMarginsF(0, 0, 0, 0)
    ));

    QPainter painter;
    if (!painter.begin(&pdfWriter)) return false;
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Draw in book units (one unit = one point) regardless of the writer's resolution.
    painter.setWindow(QRectF(QPointF(0, 0), pageSize).toRect());
    painter.setViewport(0, 0, pdfWriter.width(), pdfWriter.height());

    bool firstPage = true;
    for (const auto &spread : project.spreads) {
        QGraphicsScene scene;
        CanvasSerializer::loadIntoScene(&scene, spread.canvasData);

        const int pagesInSpread = singlePages ? 2 : 1;
        for (int side = 0; side < pagesInSpread; ++side) {
            if (!firstPage) pdfWriter.newPage();
            firstPage = false;

            const qreal offsetX = singlePages ? side * kPageWidth : 0.0;
            const QRectF source(offsetX, 0, pageSize.width(), pageSize.height());

            painter.save();
            painter.setClipRect(QRectF(QPointF(0, 0), pageSize));
            painter.translate(-offsetX, 0);
            paintSpreadPaper(painter, theme, !singlePages);
            painter.restore();

            painter.save();
            painter.setClipRect(QRectF(QPointF(0, 0), pageSize));
            scene.render(&painter, QRectF(QPointF(0, 0), pageSize), source, Qt::IgnoreAspectRatio);
            painter.restore();
        }
    }

    return painter.end();
}
