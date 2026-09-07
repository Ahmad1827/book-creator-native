#include "drawing_toolbox_dock.h"
#include <QScrollArea>

DrawingToolboxDock::DrawingToolboxDock(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void DrawingToolboxDock::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    buildIconRail();

    m_drawer = new QFrame(this);
    m_drawer->setFixedWidth(290);
    m_drawer->setStyleSheet(
        "QFrame { background: #fbf5ea; border-right: 3px solid #8d6b4f; }"
        "QLabel { color: #3b2212; font-weight: bold; }"
    );

    QVBoxLayout *drawerLayout = new QVBoxLayout(m_drawer);
    drawerLayout->setContentsMargins(10, 10, 10, 10);
    drawerLayout->setSpacing(10);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_drawerTitle = new QLabel("Tool Settings", m_drawer);
    m_drawerTitle->setStyleSheet("font-size: 13px; font-weight: 800; color: #3b2212;");
    QPushButton *btnClose = new QPushButton("✕", m_drawer);
    btnClose->setFixedSize(24, 24);
    btnClose->setStyleSheet("background: transparent; border: none; font-size: 14px; color: #7a5c43;");
    connect(btnClose, &QPushButton::clicked, [this]() {
        m_isDrawerOpen = false;
        m_drawer->setVisible(false);
    });

    headerLayout->addWidget(m_drawerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(btnClose);
    drawerLayout->addLayout(headerLayout);

    buildDrawerPages();
    drawerLayout->addWidget(m_stackedDrawer);

    mainLayout->addWidget(m_iconRail);
    mainLayout->addWidget(m_drawer);

    m_drawer->setVisible(false);
}

void DrawingToolboxDock::buildIconRail() {
    m_iconRail = new QWidget(this);
    m_iconRail->setFixedWidth(58);
    m_iconRail->setStyleSheet(
        "QWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #381f13, stop:1 #2b170c); border-right: 2px solid #1c0e07; }"
        "QPushButton { background: #fbf5ea; border: 1.5px solid #bda383; border-radius: 8px; font-weight: 800; color: #5c351f; margin: 3px 0; }"
        "QPushButton:hover { background: #ffffff; border-color: #8b5b32; color: #3b2212; }"
        "QPushButton:checked { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); border-color: #8b5b32; color: #3b2212; }"
    );

    QVBoxLayout *railLayout = new QVBoxLayout(m_iconRail);
    railLayout->setContentsMargins(6, 12, 6, 12);
    railLayout->setSpacing(6);

    auto createRailButton = [this, railLayout](const QString &text, int pageIdx, const QString &toolId, const QString &title) {
        QPushButton *btn = new QPushButton(text, m_iconRail);
        btn->setFixedSize(44, 40);
        btn->setCheckable(true);
        connect(btn, &QPushButton::clicked, [this, btn, pageIdx, toolId, title]() {
            emit toolSelected(toolId);
            if (pageIdx >= 0) {
                m_drawerTitle->setText(title);
                m_stackedDrawer->setCurrentIndex(pageIdx);
                m_isDrawerOpen = true;
                m_drawer->setVisible(true);
            } else {
                m_isDrawerOpen = false;
                m_drawer->setVisible(false);
            }
        });
        railLayout->addWidget(btn);
        return btn;
    };

    createRailButton("Sel", -1, "select", "Selection");
    createRailButton("Pen", 0, "pencil", "Sketchbook Lead");
    createRailButton("Brsh", 1, "brush", "Brushes & Eraser");
    createRailButton("Col", 2, "palette", "Color Studio");
    createRailButton("Shp", 3, "shapes", "Storybook Geometry");
    createRailButton("Stk", 4, "stickers", "Story Stamps");
    createRailButton("Lay", 5, "arrange", "Layer Ordering");

    railLayout->addStretch();
}

void DrawingToolboxDock::buildDrawerPages() {
    m_stackedDrawer = new QStackedWidget(m_drawer);

    m_stackedDrawer->addWidget(createPencilPage());
    m_stackedDrawer->addWidget(createBrushPage());
    m_stackedDrawer->addWidget(createPalettePage());
    m_stackedDrawer->addWidget(createShapesPage());
    m_stackedDrawer->addWidget(createStickersPage());
    m_stackedDrawer->addWidget(createArrangePage());
}

QWidget *DrawingToolboxDock::createPencilPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblLead = new QLabel("Graphite Grade", page);
    layout->addWidget(lblLead);

    QHBoxLayout *leadsLayout = new QHBoxLayout();
    struct LeadConfig { QString name; QString color; qreal size; qreal opacity; };
    QList<LeadConfig> leads = {
        {"HB", "#3a3430", 2.0, 0.7},
        {"2B", "#282320", 3.0, 0.85},
        {"4B", "#171412", 5.0, 0.95},
        {"Sepia", "#593c28", 3.0, 0.85}
    };

    for (const auto &lead : leads) {
        QPushButton *btn = new QPushButton(lead.name, page);
        btn->setStyleSheet("background: #f3e9d8; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 6px 0; font-weight: 800;");
        connect(btn, &QPushButton::clicked, [this, lead]() {
            emit brushColorSelected(QColor(lead.color));
            emit brushSizeSelected(lead.size);
            emit brushOpacitySelected(lead.opacity);
        });
        leadsLayout->addWidget(btn);
    }
    layout->addLayout(leadsLayout);

    QLabel *lblSize = new QLabel("Lead Thickness", page);
    QSlider *sliderSize = new QSlider(Qt::Horizontal, page);
    sliderSize->setRange(1, 18);
    sliderSize->setValue(3);
    connect(sliderSize, &QSlider::valueChanged, [this](int val) {
        emit brushSizeSelected(val);
    });

    layout->addWidget(lblSize);
    layout->addWidget(sliderSize);
    layout->addStretch();
    return page;
}

QWidget *DrawingToolboxDock::createBrushPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblType = new QLabel("Brush Medium", page);
    layout->addWidget(lblType);

    struct BrushEntry { QString name; QString id; };
    QList<BrushEntry> brushes = {
        {"Fountain Pen", "ink"},
        {"Watercolor Wash", "watercolor"},
        {"Felt Marker", "marker"},
        {"Wax Crayon", "crayon"},
        {"Stroke Eraser", "eraser"}
    };

    for (const auto &b : brushes) {
        QPushButton *btn = new QPushButton(b.name, page);
        btn->setStyleSheet("background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 8px; text-align: left; font-weight: 800;");
        connect(btn, &QPushButton::clicked, [this, b]() {
            emit brushSubtypeSelected(b.id);
        });
        layout->addWidget(btn);
    }

    QLabel *lblSize = new QLabel("Brush Thickness", page);
    QSlider *sliderSize = new QSlider(Qt::Horizontal, page);
    sliderSize->setRange(2, 70);
    sliderSize->setValue(5);
    connect(sliderSize, &QSlider::valueChanged, [this](int val) {
        emit brushSizeSelected(val);
    });

    layout->addWidget(lblSize);
    layout->addWidget(sliderSize);
    layout->addStretch();
    return page;
}

QWidget *DrawingToolboxDock::createPalettePage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    struct PaletteEntry { QString name; QStringList colors; };
    QList<PaletteEntry> collections = {
        {"Cozy Café", {"#2c211a", "#4d3a2e", "#8b6d5c", "#c68b59", "#dda15e", "#faf5ec"}},
        {"Mossy Forest", {"#1c291e", "#3a5a40", "#588157", "#819b7a", "#a3b18a", "#dad7cd"}},
        {"Sweet Macaron", {"#e07a5f", "#f4a261", "#e76f51", "#f7cad0", "#ffb3c6", "#ffffff"}},
        {"Midnight Sky", {"#0f172a", "#1e293b", "#3d5a80", "#847596", "#9d4edd", "#f3f5fa"}}
    };

    for (const auto &col : collections) {
        QLabel *lbl = new QLabel(col.name, page);
        layout->addWidget(lbl);

        QHBoxLayout *row = new QHBoxLayout();
        for (const auto &hex : col.colors) {
            QPushButton *swatch = new QPushButton(page);
            swatch->setFixedSize(26, 26);
            swatch->setStyleSheet(QString("background: %1; border: 1.5px solid #bda383; border-radius: 13px;").arg(hex));
            connect(swatch, &QPushButton::clicked, [this, hex]() {
                emit brushColorSelected(QColor(hex));
            });
            row->addWidget(swatch);
        }
        layout->addLayout(row);
    }

    QPushButton *btnPick = new QPushButton("Pick Custom Color", page);
    btnPick->setStyleSheet("background: #f3e9d8; border: 1.5px solid #bda383; border-radius: 6px; padding: 6px; font-weight: 800;");
    connect(btnPick, &QPushButton::clicked, [this]() {
        QColor col = QColorDialog::getColor(m_currentBrushColor, this, "Select Color");
        if (col.isValid()) {
            emit brushColorSelected(col);
        }
    });
    layout->addWidget(btnPick);
    layout->addStretch();
    return page;
}

QWidget *DrawingToolboxDock::createShapesPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblShapes = new QLabel("Storybook Shapes", page);
    layout->addWidget(lblShapes);

    QGridLayout *grid = new QGridLayout();
    QStringList shapes = {"star", "heart", "cloud", "moon", "flower", "speech", "thought", "circle", "rect"};

    int r = 0, c = 0;
    for (const auto &s : shapes) {
        QPushButton *btn = new QPushButton(s.toUpper(), page);
        btn->setStyleSheet("background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 8px 4px; font-weight: 800;");
        connect(btn, &QPushButton::clicked, [this, s]() {
            emit shapeAdded(s, m_shapeFill, m_shapeStroke, m_shapeStrokeWidth);
        });
        grid->addWidget(btn, r, c);
        if (++c > 2) { c = 0; ++r; }
    }
    layout->addLayout(grid);

    QLabel *lblFill = new QLabel("Fill Pigment", page);
    layout->addWidget(lblFill);
    QHBoxLayout *fillRow = new QHBoxLayout();
    QStringList fillPresets = {"#dda15e", "#c68b59", "#e07a5f", "#819b7a", "#3d5a80", "#faf5ec"};
    for (const auto &hex : fillPresets) {
        QPushButton *swatch = new QPushButton(page);
        swatch->setFixedSize(24, 24);
        swatch->setStyleSheet(QString("background: %1; border: 1.5px solid #bda383; border-radius: 12px;").arg(hex));
        connect(swatch, &QPushButton::clicked, [this, hex]() {
            m_shapeFill = QColor(hex);
        });
        fillRow->addWidget(swatch);
    }
    layout->addLayout(fillRow);

    layout->addStretch();
    return page;
}

QWidget *DrawingToolboxDock::createStickersPage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget(scroll);
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    struct StickerEntry { QString id; QString name; QString fill; QString svg; };
    QList<StickerEntry> stickers = {
        {"crown", "Crown", "#f59e0b", "M 10 70 L 20 25 L 45 45 L 50 15 L 55 45 L 80 25 L 90 70 Z"},
        {"tree", "Tree", "#15803d", "M 50 10 L 75 40 L 65 40 L 85 65 L 55 65 L 55 85 L 45 85 L 45 65 L 15 65 L 35 40 L 25 40 Z"},
        {"shroom", "Shroom", "#e11d48", "M 15 50 C 15 15 85 15 85 50 Z M 40 50 L 40 85 L 60 85 L 60 50 Z"},
        {"tea", "Teacup", "#a16207", "M 20 35 L 25 80 C 25 88 75 88 75 80 L 80 35 Z M 77 45 C 90 45 90 70 76 70"},
        {"cupcake", "Cupcake", "#fb7185", "M 30 50 L 35 85 L 65 85 L 70 50 Z M 20 50 C 20 20 80 20 80 50 Z"},
        {"boat", "Boat", "#0284c7", "M 10 65 L 90 65 L 75 85 L 25 85 Z M 48 20 L 48 60 L 15 60 Z M 52 10 L 52 60 L 82 60 Z"}
    };

    QGridLayout *grid = new QGridLayout();
    int r = 0, c = 0;
    for (const auto &stk : stickers) {
        QPushButton *btn = new QPushButton(stk.name, scrollContent);
        btn->setStyleSheet(QString("background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 12px 6px; font-weight: 800; color: %1;").arg(stk.fill));
        connect(btn, &QPushButton::clicked, [this, stk]() {
            emit stickerAdded(stk.id, stk.svg, QColor(stk.fill));
        });
        grid->addWidget(btn, r, c);
        if (++c > 1) { c = 0; ++r; }
    }
    contentLayout->addLayout(grid);
    contentLayout->addStretch();

    scroll->setWidget(scrollContent);
    layout->addWidget(scroll);
    return page;
}

QWidget *DrawingToolboxDock::createArrangePage() {
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    QPushButton *btnFront = new QPushButton("Bring to Front", page);
    QPushButton *btnBack = new QPushButton("Send to Back", page);
    QPushButton *btnDup = new QPushButton("Duplicate Item", page);
    QPushButton *btnDel = new QPushButton("Delete Item", page);

    QString style = "background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 8px; font-weight: 800; color: #3b2212;";
    btnFront->setStyleSheet(style);
    btnBack->setStyleSheet(style);
    btnDup->setStyleSheet(style);
    btnDel->setStyleSheet("background: #fee2e2; border: 1.5px solid #fca5a5; border-radius: 6px; padding: 8px; font-weight: 800; color: #b91c1c;");

    connect(btnFront, &QPushButton::clicked, this, &DrawingToolboxDock::bringForwardRequested);
    connect(btnBack, &QPushButton::clicked, this, &DrawingToolboxDock::sendBackwardRequested);
    connect(btnDup, &QPushButton::clicked, this, &DrawingToolboxDock::duplicateRequested);
    connect(btnDel, &QPushButton::clicked, this, &DrawingToolboxDock::deleteRequested);

    layout->addWidget(btnFront);
    layout->addWidget(btnBack);
    layout->addWidget(btnDup);
    layout->addWidget(btnDel);
    layout->addStretch();
    return page;
}