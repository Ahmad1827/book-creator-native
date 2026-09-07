#include "bookshelf_view.h"

BookshelfView::BookshelfView(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void BookshelfView::setupUi() {
    setStyleSheet(
        "QWidget { background: #faf5ed; color: #3b2212; font-family: sans-serif; }"
        "QLineEdit { background: #ffffff; border: 1.5px solid #dcc6ab; border-radius: 8px; padding: 7px 12px; color: #3b2212; font-size: 12px; }"
        "QLineEdit:focus { border-color: #8b5b32; }"
    );

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget *navBar = new QWidget(this);
    navBar->setFixedHeight(64);
    navBar->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #442616, stop:1 #2e180d); border-bottom: 3px solid #1c0e07;");

    QHBoxLayout *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(40, 0, 40, 0);

    QVBoxLayout *brandLayout = new QVBoxLayout();
    brandLayout->setSpacing(2);
    QLabel *lblBrand = new QLabel("Atelier Lofi", navBar);
    lblBrand->setStyleSheet("font-size: 17px; font-weight: 900; color: #faebd7;");
    QLabel *lblSub = new QLabel("Picture Book Studio", navBar);
    lblSub->setStyleSheet("font-size: 10px; color: #dfbe87; font-weight: bold;");
    brandLayout->addWidget(lblBrand);
    brandLayout->addWidget(lblSub);
    navLayout->addLayout(brandLayout);

    navLayout->addStretch();

    QPushButton *btnLoad = new QPushButton("Load Book (.json)", navBar);
    btnLoad->setStyleSheet(
        "QPushButton { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 7px 14px; font-weight: bold; color: #3b2212; font-size: 11px; }"
        "QPushButton:hover { background: #ffffff; }"
    );
    connect(btnLoad, &QPushButton::clicked, this, &BookshelfView::loadProjectRequested);
    navLayout->addWidget(btnLoad);

    QPushButton *btnCreate = new QPushButton("+ Create New Book", navBar);
    btnCreate->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); border: 1.5px solid #8c633f; border-radius: 6px; padding: 7px 16px; font-weight: 900; color: #3b2212; font-size: 11px; }"
        "QPushButton:hover { background: #faede0; }"
    );
    connect(btnCreate, &QPushButton::clicked, this, &BookshelfView::createProjectRequested);
    navLayout->addWidget(btnCreate);

    rootLayout->addWidget(navBar);

    QScrollArea *mainScroll = new QScrollArea(this);
    mainScroll->setWidgetResizable(true);
    mainScroll->setFrameShape(QFrame::NoFrame);

    QWidget *contentWidget = new QWidget(mainScroll);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(60, 32, 60, 32);
    contentLayout->setSpacing(20);

    QHBoxLayout *heroCards = new QHBoxLayout();
    heroCards->setSpacing(18);

    auto createHeroCard = [this, heroCards](const QString &title, const QString &desc, const QString &actionText, const QString &tagColor, auto clickHandler) {
        QPushButton *card = new QPushButton(this);
        card->setText(QString("%1\n%2\n\n%3").arg(title, desc, actionText));
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setFixedHeight(96);
        card->setStyleSheet(QString(
            "QPushButton { "
            "   background: #fdfaf5; "
            "   border: 1.5px solid #e2cfb9; "
            "   border-radius: 12px; "
            "   text-align: left; "
            "   padding: 16px 20px; "
            "   color: #3b2212; "
            "   font-weight: 800; "
            "   font-size: 13px; "
            "} "
            "QPushButton:hover { "
            "   border-color: #8b5b32; "
            "   background: #ffffff; "
            "}"
        ));

        connect(card, &QPushButton::clicked, clickHandler);
        heroCards->addWidget(card);
    };

    createHeroCard("Start a New Storybook", "Set up paper tone, author dedication, and decorative corner styling.", "Open Wizard →", "#8b5b32", [this]() {
        emit createProjectRequested();
    });

    createHeroCard("Load Existing Project", "Restore a saved storybook JSON file directly from your system.", "Browse Files →", "#588157", [this]() {
        emit loadProjectRequested();
    });

    contentLayout->addLayout(heroCards);

    QHBoxLayout *filterRow = new QHBoxLayout();
    m_lblCount = new QLabel("Your Bookshelf", contentWidget);
    m_lblCount->setStyleSheet("font-size: 16px; font-weight: 900; color: #3b2212;");
    filterRow->addWidget(m_lblCount);
    filterRow->addStretch();

    m_editSearch = new QLineEdit(contentWidget);
    m_editSearch->setFixedWidth(240);
    m_editSearch->setPlaceholderText("Filter by title or author...");
    connect(m_editSearch, &QLineEdit::textChanged, this, &BookshelfView::refreshGrid);
    filterRow->addWidget(m_editSearch);

    contentLayout->addLayout(filterRow);

    m_gridContainer = new QWidget(contentWidget);
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(18);

    contentLayout->addWidget(m_gridContainer);
    contentLayout->addStretch();

    mainScroll->setWidget(contentWidget);
    rootLayout->addWidget(mainScroll, 1);
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

    m_lblCount->setText(QString("Your Bookshelf (%1 saved)").arg(m_projects.size()));

    if (filtered.isEmpty()) {
        QLabel *empty = new QLabel("No story notebooks found.", m_gridContainer);
        empty->setStyleSheet("color: #7a5c43; font-size: 12px; font-style: italic; padding: 20px 0;");
        m_gridLayout->addWidget(empty, 0, 0);
        return;
    }

    int r = 0, c = 0;
    for (const auto &proj : filtered) {
        const auto &theme = getThemeById(proj.themeId);

        QFrame *card = new QFrame(m_gridContainer);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setFixedHeight(138);
        card->setStyleSheet(
            "QFrame { background: #fbf5ea; border: 1.5px solid #dcc6ab; border-radius: 10px; }"
            "QFrame:hover { border-color: #8b5b32; }"
        );

        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 16, 0);
        cardLayout->setSpacing(14);

        QFrame *spine = new QFrame(card);
        spine->setFixedWidth(14);
        spine->setStyleSheet(QString("background: %1; border-top-left-radius: 9px; border-bottom-left-radius: 9px; border: none;").arg(theme.spineColor.name()));
        cardLayout->addWidget(spine);

        QVBoxLayout *body = new QVBoxLayout();
        body->setContentsMargins(0, 12, 0, 12);
        body->setSpacing(4);

        QHBoxLayout *metaTop = new QHBoxLayout();
        QLabel *tBadge = new QLabel(theme.name, card);
        tBadge->setStyleSheet(QString("color: %1; font-weight: 800; font-size: 9px; border: 1px solid %2; border-radius: 4px; padding: 1px 5px;").arg(theme.frameColor.name(), theme.borderColor.name()));
        QLabel *dBadge = new QLabel(proj.createdAt, card);
        dBadge->setStyleSheet("color: #7a5c43; font-size: 9px;");
        metaTop->addWidget(tBadge);
        metaTop->addStretch();
        metaTop->addWidget(dBadge);
        body->addLayout(metaTop);

        QLabel *lblTitle = new QLabel(proj.title, card);
        lblTitle->setStyleSheet("font-size: 14px; font-weight: 900; color: #3b2212; border: none; margin-top: 2px;");
        lblTitle->setWordWrap(true);
        body->addWidget(lblTitle);

        QLabel *lblAuthor = new QLabel(QString("by %1").arg(proj.author), card);
        lblAuthor->setStyleSheet("font-size: 10px; color: #7a5c43; border: none;");
        body->addWidget(lblAuthor);

        body->addStretch();

        QHBoxLayout *footer = new QHBoxLayout();
        footer->setSpacing(5);

        QLabel *lblPages = new QLabel(QString("%1 pages").arg(proj.spreads.size() * 2), card);
        lblPages->setStyleSheet("font-size: 10px; font-weight: bold; color: #8b6d5c; border: none;");
        footer->addWidget(lblPages);
        footer->addStretch();

        QPushButton *btnExport = new QPushButton("Save", card);
        btnExport->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid #dcc6ab; border-radius: 4px; padding: 3px 8px; font-size: 10px; font-weight: bold; color: #3b2212; } "
            "QPushButton:hover { background: #ffffff; border-color: #8b5b32; }"
        );
        connect(btnExport, &QPushButton::clicked, [this, proj]() { emit exportJsonRequested(proj); });
        footer->addWidget(btnExport);

        QPushButton *btnDup = new QPushButton("Copy", card);
        btnDup->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid #dcc6ab; border-radius: 4px; padding: 3px 8px; font-size: 10px; font-weight: bold; color: #3b2212; } "
            "QPushButton:hover { background: #ffffff; border-color: #8b5b32; }"
        );
        connect(btnDup, &QPushButton::clicked, [this, proj]() { emit duplicateProjectRequested(proj); });
        footer->addWidget(btnDup);

        QPushButton *btnDel = new QPushButton("✕", card);
        btnDel->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid #fca5a5; border-radius: 4px; padding: 3px 7px; font-size: 10px; font-weight: bold; color: #b91c1c; } "
            "QPushButton:hover { background: #fee2e2; }"
        );
        connect(btnDel, &QPushButton::clicked, [this, proj]() { emit deleteProjectRequested(proj.id); });
        footer->addWidget(btnDel);

        QPushButton *btnOpen = new QPushButton("Open", card);
        btnOpen->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); border: 1px solid #8c633f; border-radius: 4px; padding: 3px 12px; font-size: 10px; font-weight: 900; color: #3b2212; } "
            "QPushButton:hover { background: #faede0; }"
        );
        connect(btnOpen, &QPushButton::clicked, [this, proj]() { emit openProjectRequested(proj); });
        footer->addWidget(btnOpen);

        body->addLayout(footer);
        cardLayout->addLayout(body, 1);

        m_gridLayout->addWidget(card, r, c);
        if (++c > 1) { c = 0; ++r; }
    }
}