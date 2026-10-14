#include "page_sorter_dialog.h"
#include "constants.h"
#include "spread_renderer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QPainter>
#include <QUuid>

namespace {

constexpr int kThumbWidth = 300;
constexpr int kCaptionHeight = 26;

QString newSpreadId() {
    return "s-" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace

PageSorterDialog::PageSorterDialog(const BookProject &project, const QString &currentSpreadId, QWidget *parent)
    : QDialog(parent),
      m_project(project),
      m_focusSpreadId(currentSpreadId) {

    setWindowTitle("Arrange Pages");
    resize(1080, 720);
    setStyleSheet(
        "QDialog { background: #fbf5ea; }"
        "QLabel { color: #3b2212; }"
        "QCheckBox { color: #3b2212; font-weight: 700; }"
        "QPushButton { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 7px 14px; font-weight: 800; color: #3b2212; }"
        "QPushButton:hover { background: #ffffff; border-color: #8b5b32; }"
        "QPushButton:disabled { color: #b9a58f; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    QLabel *title = new QLabel("Arrange Pages", this);
    title->setStyleSheet("font-size: 18px; font-weight: 900;");
    layout->addWidget(title);

    QLabel *hint = new QLabel("Drag spreads to reorder them, or select one and use the buttons. Double-click a spread to open it. The cover always stays first.", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #7a5c43;");
    layout->addWidget(hint);

    m_list = new QListWidget(this);
    m_list->setViewMode(QListView::ListMode);
    m_list->setFlow(QListView::LeftToRight);
    m_list->setWrapping(true);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setSpacing(10);
    m_list->setIconSize(QSize(kThumbWidth, qRound(kThumbWidth * 650.0 / 1200.0) + kCaptionHeight));
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setStyleSheet(
        "QListWidget { background: #3b2012; border: 2px solid #1c0e07; border-radius: 10px; padding: 10px; }"
        "QListWidget::item { border: 3px solid transparent; border-radius: 6px; }"
        "QListWidget::item:selected { border-color: #dfbe87; background: transparent; }"
    );
    layout->addWidget(m_list, 1);

    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, [this]() {
        // Defer so the view has finished its own drop handling first.
        QMetaObject::invokeMethod(this, [this]() {
            QListWidgetItem *current = m_list->currentItem();
            const QString id = current ? current->data(Qt::UserRole).toString() : QString();
            syncOrderFromList();
            rebuildList(id);
        }, Qt::QueuedConnection);
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_focusSpreadId = item->data(Qt::UserRole).toString();
        accept();
    });

    QHBoxLayout *actions = new QHBoxLayout();
    actions->setSpacing(8);
    QPushButton *btnEarlier = new QPushButton("◀ Move Earlier", this);
    QPushButton *btnLater = new QPushButton("Move Later ▶", this);
    QPushButton *btnDuplicate = new QPushButton("Duplicate", this);
    QPushButton *btnAdd = new QPushButton("+ Add Blank Spread", this);
    QPushButton *btnDelete = new QPushButton("Delete", this);
    btnDelete->setStyleSheet("QPushButton { background: #fee2e2; border: 1.5px solid #fca5a5; color: #b91c1c; } QPushButton:hover { background: #fecaca; }");
    actions->addWidget(btnEarlier);
    actions->addWidget(btnLater);
    actions->addSpacing(12);
    actions->addWidget(btnDuplicate);
    actions->addWidget(btnAdd);
    actions->addWidget(btnDelete);
    actions->addStretch();
    m_chkPageNumbers = new QCheckBox("Show page numbers", this);
    m_chkPageNumbers->setChecked(m_project.showPageNumbers);
    connect(m_chkPageNumbers, &QCheckBox::toggled, this, [this](bool on) {
        m_project.showPageNumbers = on;
        m_thumbCache.clear();
        rebuildList(m_list->currentItem() ? m_list->currentItem()->data(Qt::UserRole).toString() : QString());
    });
    actions->addWidget(m_chkPageNumbers);
    layout->addLayout(actions);

    connect(btnEarlier, &QPushButton::clicked, this, [this]() { moveSelected(-1); });
    connect(btnLater, &QPushButton::clicked, this, [this]() { moveSelected(1); });

    connect(btnDuplicate, &QPushButton::clicked, this, [this]() {
        const int idx = selectedIndex();
        if (idx < 0 || m_project.spreads[idx].isCover()) return;
        BookSpread copy = m_project.spreads[idx];
        copy.id = newSpreadId();
        m_project.spreads.insert(idx + 1, copy);
        rebuildList(copy.id);
    });

    connect(btnAdd, &QPushButton::clicked, this, [this]() {
        const int idx = selectedIndex();
        BookSpread spread = BookSpread::blank(newSpreadId());
        const int insertAt = idx >= 0 ? idx + 1 : m_project.spreads.size();
        m_project.spreads.insert(insertAt, spread);
        rebuildList(spread.id);
    });

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        const int idx = selectedIndex();
        if (idx < 0 || m_project.spreads[idx].isCover()) return;
        const int storySpreads = m_project.pageCount() / 2;
        if (storySpreads <= 1) {
            QMessageBox::information(this, "Delete Spread", "A book needs at least one spread of pages.");
            return;
        }
        const auto &spread = m_project.spreads[idx];
        const QString question = QString("Delete pages %1 & %2 and everything on them?\n\nThis can't be undone once you press Done.")
            .arg(spread.leftPageNum).arg(spread.rightPageNum);
        if (QMessageBox::question(this, "Delete Spread", question) != QMessageBox::Yes) return;
        m_project.spreads.removeAt(idx);
        const int next = qMin(idx, int(m_project.spreads.size()) - 1);
        rebuildList(m_project.spreads[next].id);
    });

    QHBoxLayout *footer = new QHBoxLayout();
    footer->addStretch();
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    QPushButton *btnDone = new QPushButton("Done", this);
    btnDone->setDefault(true);
    btnDone->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); border: 1.5px solid #8c633f; padding: 8px 22px; }");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnDone, &QPushButton::clicked, this, [this]() {
        if (QListWidgetItem *current = m_list->currentItem()) m_focusSpreadId = current->data(Qt::UserRole).toString();
        accept();
    });
    footer->addWidget(btnCancel);
    footer->addWidget(btnDone);
    layout->addLayout(footer);

    rebuildList(currentSpreadId);
}

QPixmap PageSorterDialog::thumbnailFor(const BookSpread &spread) {
    // Captions change with page numbers, so the cache key includes them.
    const QString key = QString("%1|%2").arg(spread.id).arg(spread.leftPageNum);
    auto it = m_thumbCache.constFind(key);
    if (it != m_thumbCache.constEnd()) return it.value();

    const BookTheme &theme = getThemeById(m_project.themeId);
    const QPixmap art = SpreadRenderer::thumbnail(theme, spread, SpreadRenderer::spreadRect(), kThumbWidth, m_project.showPageNumbers);

    QPixmap pix(art.width(), art.height() + kCaptionHeight);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.drawPixmap(0, 0, art);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(13);
    painter.setFont(font);
    painter.setPen(QColor("#dfbe87"));
    const QString caption = spread.isCover()
        ? QString("Back Cover  ·  Front Cover")
        : QString("Pages %1 – %2").arg(spread.leftPageNum).arg(spread.rightPageNum);
    painter.drawText(QRect(0, art.height(), art.width(), kCaptionHeight), Qt::AlignCenter, caption);
    painter.end();

    m_thumbCache.insert(key, pix);
    return pix;
}

void PageSorterDialog::rebuildList(const QString &selectId) {
    m_project.renumberPages();
    m_list->clear();
    for (const auto &spread : m_project.spreads) {
        // Same pixmap for the selected state, so selection doesn't tint the artwork.
        const QPixmap thumb = thumbnailFor(spread);
        QIcon icon(thumb);
        icon.addPixmap(thumb, QIcon::Selected);
        auto *item = new QListWidgetItem(icon, QString(), m_list);
        item->setData(Qt::UserRole, spread.id);
        item->setSizeHint(m_list->iconSize() + QSize(12, 12));
        if (spread.isCover()) {
            item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
        }
        if (spread.id == selectId) m_list->setCurrentItem(item);
    }
}

void PageSorterDialog::syncOrderFromList() {
    QHash<QString, BookSpread> byId;
    for (const auto &spread : m_project.spreads) byId.insert(spread.id, spread);

    QList<BookSpread> ordered;
    for (int i = 0; i < m_list->count(); ++i) {
        const QString id = m_list->item(i)->data(Qt::UserRole).toString();
        if (byId.contains(id)) ordered.append(byId.take(id));
    }
    // Anything the view lost track of goes on the end rather than disappearing.
    for (const auto &spread : m_project.spreads) {
        if (byId.contains(spread.id)) ordered.append(byId.take(spread.id));
    }
    // The cover always leads.
    for (int i = 1; i < ordered.size(); ++i) {
        if (ordered[i].isCover()) {
            ordered.move(i, 0);
            break;
        }
    }
    m_project.spreads = ordered;
}

int PageSorterDialog::selectedIndex() const {
    QListWidgetItem *current = m_list->currentItem();
    if (!current) return -1;
    const QString id = current->data(Qt::UserRole).toString();
    for (int i = 0; i < m_project.spreads.size(); ++i) {
        if (m_project.spreads[i].id == id) return i;
    }
    return -1;
}

void PageSorterDialog::moveSelected(int delta) {
    const int idx = selectedIndex();
    if (idx < 0 || m_project.spreads[idx].isCover()) return;
    const int target = idx + delta;
    const int firstStory = m_project.hasCover() ? 1 : 0;
    if (target < firstStory || target >= m_project.spreads.size()) return;
    const QString id = m_project.spreads[idx].id;
    m_project.spreads.move(idx, target);
    rebuildList(id);
}

QList<BookSpread> PageSorterDialog::spreads() const {
    return m_project.spreads;
}

bool PageSorterDialog::showPageNumbers() const {
    return m_project.showPageNumbers;
}

QString PageSorterDialog::focusSpreadId() const {
    return m_focusSpreadId;
}
