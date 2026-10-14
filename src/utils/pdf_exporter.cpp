#include "pdf_exporter.h"
#include "spread_renderer.h"
#include <QJsonArray>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>

namespace {

struct PdfPage {
    const BookSpread *spread;
    QRectF source;
};

// The trim rectangle grown into the bleed on its outer edges only; the spine side is
// glued into the binding, so it never gets bleed.
QRectF withBleed(const QRectF &trim, qreal bleed, bool leftOuter, bool rightOuter) {
    return trim.adjusted(leftOuter ? -bleed : 0, -bleed, rightOuter ? bleed : 0, bleed);
}

} // namespace

QSizeF PdfExporter::pageSizeInches(const BookProject &project, const Options &options) {
    const BookFormat &format = project.format();
    const bool single = options.layout == Layout::SinglePages;
    const qreal bleedW = options.bleed ? (single ? 1 : 2) * BookFormat::kBleedIn : 0.0;
    const qreal bleedH = options.bleed ? 2 * BookFormat::kBleedIn : 0.0;
    return QSizeF(format.trimWidthIn * (single ? 1 : 2) + bleedW, format.trimHeightIn + bleedH);
}

bool PdfExporter::exportProjectToPdf(
    const BookProject &project,
    const BookTheme &theme,
    const Options &options,
    const QString &outputPath
) {
    if (project.spreads.isEmpty()) return false;

    const BookFormat &format = project.format();
    const qreal pageHeight = project.pageHeight();
    const qreal bleed = options.bleed ? format.bleedUnits() : 0.0;
    const bool singlePages = options.layout == Layout::SinglePages;
    const QRectF left = SpreadRenderer::leftPageRect(pageHeight);
    const QRectF right = SpreadRenderer::rightPageRect(pageHeight);

    // As a printed book: front cover, story pages, back cover.
    QList<PdfPage> pages;
    const BookSpread *cover = project.hasCover() ? &project.spreads.first() : nullptr;
    if (singlePages) {
        if (cover) pages.append({cover, withBleed(right, bleed, false, true)});
        for (const auto &spread : project.spreads) {
            if (spread.isCover()) continue;
            pages.append({&spread, withBleed(left, bleed, true, false)});
            pages.append({&spread, withBleed(right, bleed, false, true)});
        }
        if (cover) pages.append({cover, withBleed(left, bleed, true, false)});
    } else {
        for (const auto &spread : project.spreads) {
            pages.append({&spread, withBleed(SpreadRenderer::spreadRect(pageHeight), bleed, true, true)});
        }
    }

    const QSizeF unitsSize = pages.first().source.size();
    const QSizeF pointsSize = unitsSize * format.pointsPerUnit();

    QPdfWriter pdfWriter(outputPath);
    pdfWriter.setResolution(300);
    pdfWriter.setTitle(project.title);
    pdfWriter.setCreator(project.author);
    pdfWriter.setPageLayout(QPageLayout(
        QPageSize(pointsSize, QPageSize::Point, QString(), QPageSize::ExactMatch),
        QPageLayout::Portrait,
        QMarginsF(0, 0, 0, 0)
    ));

    QPainter painter;
    if (!painter.begin(&pdfWriter)) return false;
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Draw in canvas units; the viewport scales them to the trim size.
    const QRectF target(QPointF(0, 0), unitsSize);
    painter.setWindow(target.toRect());
    painter.setViewport(0, 0, pdfWriter.width(), pdfWriter.height());

    for (int i = 0; i < pages.size(); ++i) {
        if (i > 0) pdfWriter.newPage();
        const auto &page = pages[i];
        SpreadRenderer::paint(painter, theme, *page.spread, pageHeight, page.source, target,
                              project.showPageNumbers, !singlePages);
    }

    return painter.end();
}

QStringList PdfExporter::lowResolutionPictures(const BookProject &project, qreal minDpi) {
    QStringList warnings;
    const qreal unitsPerInch = project.format().unitsPerInch();
    for (const auto &spread : project.spreads) {
        for (const auto &val : spread.canvasData["items"].toArray()) {
            const QJsonObject obj = val.toObject();
            if (obj["type"].toString() != "image") continue;
            // One image pixel covers `scale` units, so pixels per inch = unitsPerInch / scale.
            const qreal scale = obj["scale"].toDouble(1.0);
            const qreal dpi = unitsPerInch / qMax(0.0001, scale);
            if (dpi >= minDpi) continue;
            const QString where = spread.isCover()
                ? QString("The cover")
                : QString("Pages %1–%2").arg(spread.leftPageNum).arg(spread.rightPageNum);
            warnings << QString("%1: a picture will print at about %2 dpi").arg(where).arg(qRound(dpi));
        }
    }
    return warnings;
}
