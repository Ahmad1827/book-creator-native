#pragma once

#include <QDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include "types.h"
#include "pdf_exporter.h"

// Print options before saving a PDF: layout, printer bleed, and a check for blurry pictures.
class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(const BookProject &project, QWidget *parent = nullptr);

    PdfExporter::Options options() const;

private:
    void updateSummary();

    BookProject m_project;
    QRadioButton *m_radioSingle;
    QRadioButton *m_radioSpreads;
    QCheckBox *m_chkBleed;
    QLabel *m_lblSummary;
};
