#include "pdf_exporter.h"
#include "spread_renderer.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>

namespace {

struct PdfPage {
    const BookSpread *spread;
    QRectF source;
};

} // namespace

bool PdfExporter::exportProjectToPdf(
    const BookProject &project,
    const BookTheme &theme,
    Layout layout,
    const QString &outputPath
) {
    if (project.spreads.isEmpty()) return false;

    const bool singlePages = layout == Layout::SinglePages;

    // As a printed book: front cover, story pages, back cover.
    QList<PdfPage> pages;
    const BookSpread *cover = project.hasCover() ? &project.spreads.first() : nullptr;
    if (singlePages) {
        if (cover) pages.append({cover, SpreadRenderer::rightPageRect()});
        for (const auto &spread : project.spreads) {
            if (spread.isCover()) continue;
            pages.append({&spread, SpreadRenderer::leftPageRect()});
            pages.append({&spread, SpreadRenderer::rightPageRect()});
        }
        if (cover) pages.append({cover, SpreadRenderer::leftPageRect()});
    } else {
        for (const auto &spread : project.spreads) {
            pages.append({&spread, SpreadRenderer::spreadRect()});
        }
    }

    const QSizeF pageSize = pages.first().source.size();

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

    for (int i = 0; i < pages.size(); ++i) {
        if (i > 0) pdfWriter.newPage();
        const auto &page = pages[i];
        SpreadRenderer::paint(painter, theme, *page.spread, page.source, QRectF(QPointF(0, 0), pageSize),
                              project.showPageNumbers, !singlePages);
    }

    return painter.end();
}
