#include "bookshelf_view.h"
#include "spread_renderer.h"
#include "sketch.h"
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <functional>

namespace {

constexpr int kCardWidth = 240;
constexpr int kCardHeight = 372;
constexpr int kCardSpacing = 28;
const QRectF kCoverArea(22, 16, 196, 222);

// One book on the shelf: its front cover, slightly tilted like a real book left on a table.
class BookCard : public QWidget {
public:
    BookCard(const BookProject &project, int index, QWidget *parent)
        : QWidget(parent),
          m_project(project),
          m_tilt(index % 2 == 0 ? -2.2 : 1.8) {
        setFixedSize(kCardWidth, kCardHeight);
        setMouseTracking(true);

        const BookTheme &theme = getThemeById(project.themeId);
        if (project.hasCover()) {
            const qreal h = project.pageHeight();
            m_cover = SpreadRenderer::thumbnail(theme, project.spreads.first(), h,
                                                SpreadRenderer::rightPageRect(h), 360, false);
        }

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, int(kCoverArea.bottom()) + 16, 10, 6);
        layout->setSpacing(2);

        QLabel *title = Sketch::heading(project.title, this, 23);
        title->setAlignment(Qt::AlignHCenter);
        title->setWordWrap(true);
        layout->addWidget(title);

        QLabel *byline = Sketch::hint(QString("by %1  ·  %2 pages").arg(project.author).arg(project.pageCount()), this);
        byline->setAlignment(Qt::AlignHCenter);
        layout->addWidget(byline);
        layout->addStretch();

        QHBoxLayout *actions = new QHBoxLayout();
        actions->setSpacing(6);
        actions->addStretch();
        m_btnOpen = new QPushButton("Open", this);
        m_btnOpen->setProperty("role", "primary");
        m_btnMore = new QPushButton("⋯", this);
        m_btnMore->setToolTip("Copy, save to a file, or delete");
        m_btnMore->setFixedWidth(52);
        actions->addWidget(m_btnOpen);
        actions->addWidget(m_btnMore);
        actions->addStretch();
        layout->addLayout(actions);
    }

    std::function<void()> onOpen;
    std::function<void()> onCopy;
    std::function<void()> onSave;
    std::function<void()> onDelete;

    void wire() {
        connect(m_btnOpen, &QPushButton::clicked, this, [this]() { if (onOpen) onOpen(); });
        connect(m_btnMore, &QPushButton::clicked, this, [this]() {
            QMenu menu(this);
            menu.addAction("Make a copy", this, [this]() { if (onCopy) onCopy(); });
            menu.addAction("Save to a file…", this, [this]() { if (onSave) onSave(); });
            menu.addSeparator();
            menu.addAction("Delete this book", this, [this]() { if (onDelete) onDelete(); });
            menu.exec(m_btnMore->mapToGlobal(QPoint(0, m_btnMore->height())));
        });
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const BookTheme &theme = getThemeById(m_project.themeId);
        // Fit the cover's proportions inside the cover area.
        const qreal aspect = m_cover.isNull() ? 650.0 / 600.0 : qreal(m_cover.height()) / m_cover.width();
        QRectF cover(0, 0, kCoverArea.width() - 16, (kCoverArea.width() - 16) * aspect);
        if (cover.height() > kCoverArea.height() - 10) {
            cover = QRectF(0, 0, (kCoverArea.height() - 10) / aspect, kCoverArea.height() - 10);
        }
        cover.moveCenter(kCoverArea.center() + QPointF(0, m_hover ? -3 : 0));

        p.save();
        p.translate(cover.center());
        p.rotate(m_hover ? m_tilt * 0.4 : m_tilt);
        p.translate(-cover.center());

        const quint32 seed = Sketch::seedFor(cover.size(), qHash(m_project.id));
        const QPainterPath outline = Sketch::wobblyRoundedRect(cover, 4, seed, 1.0);
        QColor shadow = Sketch::kInk;
        shadow.setAlpha(m_hover ? 60 : 45);
        p.fillPath(outline.translated(m_hover ? 8 : 6, m_hover ? 10 : 7), shadow);

        if (!m_cover.isNull()) {
            p.save();
            p.setClipPath(outline);
            p.drawPixmap(cover, m_cover, m_cover.rect());
            p.restore();
        } else {
            p.fillPath(outline, theme.paperBg);
            p.setPen(theme.inkColor);
            p.setFont(Sketch::titleFont(22));
            p.drawText(cover.adjusted(24, 20, -12, -20), Qt::AlignCenter | Qt::TextWordWrap, m_project.title);
        }

        // The spine, painted along the left edge in the book's theme colour.
        QRectF spine(cover.left(), cover.top(), 16, cover.height());
        const QPainterPath spinePath = Sketch::wobblyRoundedRect(spine, 3, seed + 1, 0.6);
        Sketch::drawWash(&p, spinePath, theme.spineColor, QPointF(0.5, 0.5));
        Sketch::drawInk(&p, Sketch::wobblyLine(spine.topRight(), spine.bottomRight(), seed + 2, 0.6), Sketch::kInk, 1.4);
        Sketch::drawInk(&p, outline, Sketch::kInk, 2.4);
        p.restore();
    }

    void enterEvent(QEnterEvent *) override { m_hover = true; update(); }
    void leaveEvent(QEvent *) override { m_hover = false; update(); unsetCursor(); }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (kCoverArea.contains(event->position())) setCursor(Qt::PointingHandCursor);
        else unsetCursor();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && kCoverArea.contains(event->position()) && onOpen) onOpen();
    }

private:
    BookProject m_project;
    QPixmap m_cover;
    qreal m_tilt;
    bool m_hover = false;
    QPushButton *m_btnOpen;
    QPushButton *m_btnMore;
};

// The first tile on the shelf: an invitation to start a new book.
class NewBookCard : public QWidget {
public:
    explicit NewBookCard(QWidget *parent)
        : QWidget(parent) {
        setFixedSize(kCardWidth, kCardHeight);
        setCursor(Qt::PointingHandCursor);
        setToolTip("Start a brand new picture book");
    }

    std::function<void()> onClick;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF area = QRectF(kCoverArea).adjusted(8, 4, -8, -4);
        if (m_hover) area.translate(0, -3);
        const quint32 seed = Sketch::seedFor(area.size(), 99);
        const QPainterPath outline = Sketch::wobblyRoundedRect(area, 14, seed, 1.4);
        QColor wash = Sketch::kYellow;
        wash.setAlpha(m_hover ? 150 : 90);
        Sketch::drawWash(&p, outline, wash, QPointF(3, 3));
        QPen dashed(Sketch::kInk, 2.2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        dashed.setDashPattern({4, 3});
        p.setPen(dashed);
        p.setBrush(Qt::NoBrush);
        p.drawPath(outline);

        p.setPen(Sketch::kInk);
        p.setFont(Sketch::titleFont(78));
        p.drawText(area.adjusted(0, -20, 0, -20), Qt::AlignCenter, "+");
        p.setFont(Sketch::headingFont(24));
        p.drawText(QRectF(0, area.bottom() + 14, width(), 70), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, "Make a new book!");
    }

    void enterEvent(QEnterEvent *) override { m_hover = true; update(); }
    void leaveEvent(QEvent *) override { m_hover = false; update(); }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) onClick();
    }

private:
    bool m_hover = false;
};

} // namespace

BookshelfView::BookshelfView(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void BookshelfView::setupUi() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *header = new SketchPanel(this, Sketch::kPaper, SketchPanel::BottomEdge);
    header->setDoodles(true);
    header->setFixedHeight(128);

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(48, 10, 40, 14);
    headerLayout->setSpacing(10);

    QVBoxLayout *brandLayout = new QVBoxLayout();
    brandLayout->setSpacing(0);
    brandLayout->addStretch();
    brandLayout->addWidget(Sketch::title("Storybook Studio", header, 48));
    QLabel *tagline = Sketch::hint("Whizzpopping picture books, made by hand", header);
    tagline->setFont(Sketch::bodyFont(19));
    brandLayout->addWidget(tagline);
    brandLayout->addStretch();
    headerLayout->addLayout(brandLayout);

    headerLayout->addStretch();

    QPushButton *btnSettings = new QPushButton("Settings", header);
    btnSettings->setProperty("role", "ghost");
    connect(btnSettings, &QPushButton::clicked, this, &BookshelfView::settingsRequested);
    headerLayout->addWidget(btnSettings);

    QPushButton *btnLoad = new QPushButton("Open a Book File", header);
    btnLoad->setToolTip("Open a book saved with \"Save to a file…\"");
    connect(btnLoad, &QPushButton::clicked, this, &BookshelfView::loadProjectRequested);
    headerLayout->addWidget(btnLoad);

    QPushButton *btnCreate = new QPushButton("+ New Book", header);
    btnCreate->setProperty("role", "primary");
    btnCreate->setFont(Sketch::headingFont(22));
    connect(btnCreate, &QPushButton::clicked, this, &BookshelfView::createProjectRequested);
    headerLayout->addWidget(btnCreate);

    rootLayout->addWidget(header);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new SketchPanel(m_scroll, Sketch::kDesk);
    content->setDoodles(true);
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(56, 26, 56, 40);
    contentLayout->setSpacing(18);

    QHBoxLayout *filterRow = new QHBoxLayout();
    m_lblCount = Sketch::heading("Your Bookshelf", content, 32);
    filterRow->addWidget(m_lblCount);
    filterRow->addStretch();

    m_editSearch = new QLineEdit(content);
    m_editSearch->setFixedWidth(280);
    m_editSearch->setPlaceholderText("Find a book…");
    connect(m_editSearch, &QLineEdit::textChanged, this, &BookshelfView::refreshGrid);
    filterRow->addWidget(m_editSearch);
    contentLayout->addLayout(filterRow);

    m_gridContainer = new QWidget(content);
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setHorizontalSpacing(kCardSpacing);
    m_gridLayout->setVerticalSpacing(kCardSpacing);
    m_gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    contentLayout->addWidget(m_gridContainer);
    contentLayout->addStretch();

    m_scroll->setWidget(content);
    rootLayout->addWidget(m_scroll, 1);
}

int BookshelfView::columnsForWidth() const {
    // The scroll area's viewport lags behind when the window first opens, so measure ourselves.
    const int available = width() - 112 - 16;
    return qMax(1, (available + kCardSpacing) / (kCardWidth + kCardSpacing));
}

void BookshelfView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (columnsForWidth() != m_columns) refreshGrid();
}

void BookshelfView::setProjects(const QList<BookProject> &projects) {
    m_projects = projects;
    refreshGrid();
}

void BookshelfView::refreshGrid() {
    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QString query = m_editSearch->text().trimmed().toLower();
    QList<BookProject> filtered;
    for (const auto &p : m_projects) {
        if (query.isEmpty() || p.title.toLower().contains(query) || p.author.toLower().contains(query)) {
            filtered.append(p);
        }
    }

    m_lblCount->setText(m_projects.size() == 1 ? QString("Your Bookshelf (1 book)")
                                               : QString("Your Bookshelf (%1 books)").arg(m_projects.size()));

    m_columns = columnsForWidth();
    int index = 0;
    auto place = [this, &index](QWidget *w) {
        m_gridLayout->addWidget(w, index / m_columns, index % m_columns);
        ++index;
    };

    if (query.isEmpty()) {
        auto *newCard = new NewBookCard(m_gridContainer);
        newCard->onClick = [this]() { emit createProjectRequested(); };
        place(newCard);
    }

    for (const auto &proj : filtered) {
        auto *card = new BookCard(proj, index, m_gridContainer);
        card->onOpen = [this, proj]() { emit openProjectRequested(proj); };
        card->onCopy = [this, proj]() { emit duplicateProjectRequested(proj); };
        card->onSave = [this, proj]() { emit exportJsonRequested(proj); };
        card->onDelete = [this, id = proj.id]() { emit deleteProjectRequested(id); };
        card->wire();
        place(card);
    }

    if (!query.isEmpty() && filtered.isEmpty()) {
        place(Sketch::hint("No books match that. Try another word!", m_gridContainer));
    }
}
