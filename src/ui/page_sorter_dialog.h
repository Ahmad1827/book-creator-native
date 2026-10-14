#pragma once

#include <QDialog>
#include <QListWidget>
#include <QCheckBox>
#include <QHash>
#include <QPixmap>
#include "types.h"

// A bird's-eye view of the book: reorder, duplicate, add and delete spreads.
class PageSorterDialog : public QDialog {
    Q_OBJECT

public:
    PageSorterDialog(const BookProject &project, const QString &currentSpreadId, QWidget *parent = nullptr);

    QList<BookSpread> spreads() const;
    bool showPageNumbers() const;
    // The spread to show after closing (the one she double-clicked or last selected).
    QString focusSpreadId() const;

private:
    void rebuildList(const QString &selectId);
    void syncOrderFromList();
    int selectedIndex() const;
    QPixmap thumbnailFor(const BookSpread &spread);
    void moveSelected(int delta);

    BookProject m_project;
    QListWidget *m_list;
    QCheckBox *m_chkPageNumbers;
    QHash<QString, QPixmap> m_thumbCache;
    QString m_focusSpreadId;
};
