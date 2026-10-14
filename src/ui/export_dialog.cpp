#include "export_dialog.h"
#include "sketch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>

ExportDialog::ExportDialog(const BookProject &project, QWidget *parent)
    : QDialog(parent),
      m_project(project) {
    setWindowTitle("Print Book (PDF)");
    setMinimumWidth(660);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    QLabel *title = new QLabel("Print Book (PDF)", this);
    title->setFont(Sketch::titleFont(32));
    layout->addWidget(title);

    QLabel *size = new QLabel(QString("Book size: <b>%1</b> &nbsp;·&nbsp; %2 pages plus covers")
                                  .arg(project.format().name).arg(project.pageCount()), this);
    layout->addWidget(size);

    QGroupBox *layoutBox = new QGroupBox("Layout", this);
    QVBoxLayout *layoutBoxLayout = new QVBoxLayout(layoutBox);
    m_radioSingle = new QRadioButton("Single pages, for printing (front cover first, back cover last)", layoutBox);
    m_radioSpreads = new QRadioButton("Two-page spreads, side by side as on screen", layoutBox);
    m_radioSingle->setChecked(true);
    layoutBoxLayout->addWidget(m_radioSingle);
    layoutBoxLayout->addWidget(m_radioSpreads);
    layout->addWidget(layoutBox);

    QGroupBox *printBox = new QGroupBox("Printer", this);
    QVBoxLayout *printLayout = new QVBoxLayout(printBox);
    m_chkBleed = new QCheckBox("Add bleed for a print shop (1/8 inch past the edges)", printBox);
    QLabel *bleedNote = new QLabel(
        "Print shops trim a sliver off each page. Bleed extends the paper colour and any full-page "
        "pictures past the edge so nothing white shows after trimming. Leave it off for home printing.", printBox);
    bleedNote->setWordWrap(true);
    Sketch::styleHint(bleedNote);
    printLayout->addWidget(m_chkBleed);
    printLayout->addWidget(bleedNote);
    layout->addWidget(printBox);

    m_lblSummary = new QLabel(this);
    m_lblSummary->setFont(Sketch::headingFont(19));
    layout->addWidget(m_lblSummary);

    const QStringList lowRes = PdfExporter::lowResolutionPictures(project);
    if (!lowRes.isEmpty()) {
        QStringList shown = lowRes.mid(0, 6);
        if (lowRes.size() > shown.size()) shown << QString("…and %1 more").arg(lowRes.size() - shown.size());
        QLabel *warning = new QLabel(
            "<b>Some pictures may print blurry</b> (under 150 dpi). Shrinking them on the page, or scanning at a "
            "higher resolution, will help:<br>• " + shown.join("<br>• "), this);
        warning->setWordWrap(true);
        // A pinned-on note so the warning stands out on the page.
        auto *note = new SketchPanel(this, QColor("#ffe8c2"), SketchPanel::TopEdge | SketchPanel::BottomEdge | SketchPanel::LeftEdge | SketchPanel::RightEdge);
        QVBoxLayout *noteLayout = new QVBoxLayout(note);
        noteLayout->setContentsMargins(14, 10, 14, 12);
        noteLayout->addWidget(warning);
        layout->addWidget(note);
    }

    layout->addStretch();
    QHBoxLayout *footer = new QHBoxLayout();
    footer->addStretch();
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    QPushButton *btnExport = new QPushButton("Save PDF…", this);
    btnExport->setDefault(true);
    btnExport->setProperty("role", "primary");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnExport, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(btnCancel);
    footer->addWidget(btnExport);
    layout->addLayout(footer);

    connect(m_radioSingle, &QRadioButton::toggled, this, &ExportDialog::updateSummary);
    connect(m_chkBleed, &QCheckBox::toggled, this, &ExportDialog::updateSummary);
    updateSummary();
}

PdfExporter::Options ExportDialog::options() const {
    PdfExporter::Options opts;
    opts.layout = m_radioSingle->isChecked() ? PdfExporter::Layout::SinglePages : PdfExporter::Layout::Spreads;
    opts.bleed = m_chkBleed->isChecked();
    return opts;
}

void ExportDialog::updateSummary() {
    const QSizeF inches = PdfExporter::pageSizeInches(m_project, options());
    m_lblSummary->setText(QString("Each PDF page will be %1 × %2 inches.")
                              .arg(inches.width(), 0, 'g', 4).arg(inches.height(), 0, 'g', 4));
}
