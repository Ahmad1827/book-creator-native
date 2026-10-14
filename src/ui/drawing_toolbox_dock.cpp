#include "drawing_toolbox_dock.h"
#include "sketch.h"
#include <QScrollArea>
#include <QListWidget>
#include <QSvgRenderer>
#include <QPainter>

DrawingToolboxDock::DrawingToolboxDock(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

QIcon DrawingToolboxDock::createSvgIcon(const QString &pathD, const QString &fillColor) {
    QString svg = QString(
        "<svg viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"%1\" fill=\"%2\" />"
        "</svg>"
    ).arg(pathD, fillColor);

    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    renderer.render(&painter);
    painter.end();

    return QIcon(pix);
}

void DrawingToolboxDock::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    buildIconRail();

    m_drawer = new SketchPanel(this, Sketch::kCard, SketchPanel::RightEdge);
    m_drawer->setFixedWidth(0);

    QVBoxLayout *drawerLayout = new QVBoxLayout(m_drawer);
    drawerLayout->setContentsMargins(14, 14, 14, 14);
    drawerLayout->setSpacing(12);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_drawerTitle = new QLabel("Tool Settings", m_drawer);
    m_drawerTitle->setFont(Sketch::headingFont(26));
    QPushButton *btnClose = new QPushButton("✕", m_drawer);
    btnClose->setFixedSize(24, 24);
    btnClose->setProperty("role", "ghost");
    connect(btnClose, &QPushButton::clicked, [this]() {
        animateDrawer(false);
    });

    headerLayout->addWidget(m_drawerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(btnClose);
    drawerLayout->addLayout(headerLayout);

    buildDrawerPages();
    drawerLayout->addWidget(m_stackedDrawer, 1);

    mainLayout->addWidget(m_iconRail);
    mainLayout->addWidget(m_drawer);
}

void DrawingToolboxDock::animateDrawer(bool open) {
    m_isDrawerOpen = open;
    QPropertyAnimation *anim = new QPropertyAnimation(m_drawer, "minimumWidth", this);
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(m_drawer->width());
    anim->setEndValue(open ? 372 : 0);

    connect(anim, &QPropertyAnimation::valueChanged, [this](const QVariant &val) {
        m_drawer->setMaximumWidth(val.toInt());
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void DrawingToolboxDock::buildIconRail() {
    auto *rail = new SketchPanel(this, Sketch::kPaperDeep, SketchPanel::RightEdge);
    rail->setDoodles(true);
    m_iconRail = rail;
    m_iconRail->setFixedWidth(70);

    QVBoxLayout *railLayout = new QVBoxLayout(m_iconRail);
    railLayout->setContentsMargins(6, 12, 8, 12);
    railLayout->setSpacing(6);

    m_railGroup = new QButtonGroup(this);
    m_railGroup->setExclusive(true);

    auto createRailButton = [this, railLayout](const QString &svgPath, int pageIdx, const QString &toolId, const QString &title, const QString &tooltip, const QColor &wash) {
        QPushButton *btn = new QPushButton(m_iconRail);
        btn->setFixedSize(56, 52);
        btn->setCheckable(true);
        btn->setProperty("role", "rail");
        btn->setProperty("wash", wash);
        btn->setIcon(createSvgIcon(svgPath));
        btn->setIconSize(QSize(26, 26));
        btn->setToolTip(tooltip);

        m_railGroup->addButton(btn);

        connect(btn, &QPushButton::clicked, [this, pageIdx, toolId, title]() {
            // The pencil and the brush drawer share one canvas brush, so re-assert which one is in hand.
            if (toolId == "pencil") emit brushSubtypeSelected("pencil");
            else if (toolId == "brush") emit brushSubtypeSelected(m_selectedSubtype);
            emit toolSelected(toolId);
            if (pageIdx >= 0) {
                m_drawerTitle->setText(title);
                m_stackedDrawer->setCurrentIndex(pageIdx);
                animateDrawer(true);
            } else {
                animateDrawer(false);
            }
        });

        railLayout->addWidget(btn);
        return btn;
    };

    createRailButton("M4 2l6 17 3-5 5 7 2-1-5-7 7-2z", -1, "select", "Select & Move", "Select, move, resize and turn things", Sketch::kSky);
    auto *btnDraw = createRailButton("M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z", 0, "pencil", "Pencils", "Pencils for sketching", Sketch::kYellow);
    createRailButton("M7 14c-1.66 0-3 1.34-3 3 0 1.31-1.16 2-2 2 .92 1.22 2.49 2 4 2 2.21 0 4-1.79 4-4 0-1.66-1.34-3-3-3zm13.71-9.71a1 1 0 0 0-1.42 0l-9.05 9.06 2.83 2.83 9.06-9.06a1 1 0 0 0 0-1.41l-1.42-1.42z", 1, "brush", "Brushes & Eraser", "Brushes, washes and the eraser", Sketch::kTeal);
    createRailButton("M5 4v3h5.5v12h3V7H19V4z", 6, "text", "Story Words", "Write the story's words", Sketch::kTomato.lighter(125));
    createRailButton("M12 3c-4.97 0-9 4.03-9 9 0 2.12.74 4.07 1.97 5.61L4.35 19A1 1 0 0 0 5.2 20.5c1.88 0 3.32-.97 4.14-2.07.82.37 1.73.57 2.66.57 4.97 0 9-4.03 9-9s-4.03-9-9-9zm-5.5 8c-.83 0-1.5-.67-1.5-1.5S5.67 8 6.5 8s1.5.67 1.5 1.5S7.33 11 6.5 11zm3-4c-.83 0-1.5-.67-1.5-1.5S8.67 4 9.5 4s1.5.67 1.5 1.5S10.33 7 9.5 7zm5 0c-.83 0-1.5-.67-1.5-1.5S13.67 4 14.5 4s1.5.67 1.5 1.5S15.33 7 14.5 7zm3 4c-.83 0-1.5-.67-1.5-1.5S16.67 8 17.5 8s1.5.67 1.5 1.5-.67 1.5-1.5 1.5z", 2, "palette", "Paint Pots", "Pick colours", Sketch::kPink);
    createRailButton("M12 2l4.5 9h-9zM3 13.5h7v7H3zm11 0a4 4 0 1 0 0 8 4 4 0 0 0 0-8z", 3, "shapes", "Shapes & Bubbles", "Shapes and speech bubbles", Sketch::kGreen);
    createRailButton("M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm3.5-9c.83 0 1.5-.67 1.5-1.5S16.33 8 15.5 8 14 8.67 14 9.5s.67 1.5 1.5 1.5zm-7 0c.83 0 1.5-.67 1.5-1.5S9.33 8 8.5 8 7 8.67 7 9.5 7.67 11 8.5 11zm3.5 6.5c2.33 0 4.31-1.46 5.11-3.5H6.89c.8 2.04 2.78 3.5 5.11 3.5z", 4, "stickers", "Stamps", "Little picture stamps", Sketch::kOrange);
    createRailButton("M11.99 18.54l-7.37-5.73L3 14.07l9 7 9-7-1.63-1.27-7.38 5.74zM12 16l7.36-5.73L21 9.07l-9-7-9 7 1.63 1.27L12 16z", 5, "arrange", "Front & Back", "Bring to front, send to back", Sketch::kLilac);

    btnDraw->setChecked(true);
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
    m_stackedDrawer->addWidget(createTextPage());
}

QWidget *DrawingToolboxDock::createPencilPage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);
    layout->setSpacing(10);

    QLabel *lblLead = new QLabel("GRAPHITE GRADE", page);
    Sketch::styleSection(lblLead);
    layout->addWidget(lblLead);

    QHBoxLayout *leadsLayout = new QHBoxLayout();
    leadsLayout->setSpacing(6);
    m_leadGroup = new QButtonGroup(page);
    m_leadGroup->setExclusive(true);

    struct LeadConfig { QString name; QString color; qreal size; qreal opacity; };
    QList<LeadConfig> leads = {
        {"HB", "#3a3430", 2.0, 0.7},
        {"2B", "#282320", 3.0, 0.85},
        {"4B", "#171412", 5.0, 0.95},
        {"Sepia", "#593c28", 3.0, 0.85}
    };

    for (int i = 0; i < leads.size(); ++i) {
        const auto &lead = leads[i];
        QPushButton *btn = new QPushButton(lead.name, page);
        btn->setCheckable(true);
        btn->setProperty("role", "chip");
        m_leadGroup->addButton(btn, i);
        connect(btn, &QPushButton::clicked, [this, lead]() {
            emit brushColorSelected(QColor(lead.color));
            emit brushSizeSelected(lead.size);
            emit brushOpacitySelected(lead.opacity);
        });
        leadsLayout->addWidget(btn);
        if (i == 1) btn->setChecked(true);
    }
    layout->addLayout(leadsLayout);

    QLabel *lblSize = new QLabel("LEAD THICKNESS", page);
    Sketch::styleSection(lblSize);
    layout->addWidget(lblSize);

    QSlider *sliderSize = new QSlider(Qt::Horizontal, page);
    sliderSize->setRange(1, 18);
    sliderSize->setValue(3);
    connect(sliderSize, &QSlider::valueChanged, [this](int val) {
        emit brushSizeSelected(val);
    });

    layout->addWidget(sliderSize);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}

QWidget *DrawingToolboxDock::createBrushPage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(6, 6, 12, 6);
    layout->setSpacing(8);

    QLabel *lblType = new QLabel("BRUSH MEDIUM", page);
    Sketch::styleSection(lblType);
    layout->addWidget(lblType);

    m_brushGroup = new QButtonGroup(page);
    m_brushGroup->setExclusive(true);

    struct BrushEntry { QString name; QString id; QString desc; };
    QList<BrushEntry> brushes = {
        {"Fountain Pen", "ink", "Crisp linework"},
        {"Watercolor Wash", "watercolor", "Soft translucent fill"},
        {"Felt Marker", "marker", "Rich vivid color"},
        {"Wax Crayon", "crayon", "Textured granulate"},
        {"Stroke Eraser", "eraser", "Cuts ink strokes directly"}
    };

    for (int i = 0; i < brushes.size(); ++i) {
        const auto &b = brushes[i];
        QPushButton *btn = new QPushButton(page);
        btn->setCheckable(true);
        btn->setText(QString("%1\n%2").arg(b.name, b.desc));
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setProperty("role", "card");

        m_brushGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, [this, b]() {
            m_selectedSubtype = b.id;
            emit brushSubtypeSelected(b.id);
        });

        layout->addWidget(btn);
        if (b.id == "ink") btn->setChecked(true);
    }

    QLabel *lblSize = new QLabel("BRUSH THICKNESS", page);
    Sketch::styleSection(lblSize);
    layout->addWidget(lblSize);

    QSlider *sliderSize = new QSlider(Qt::Horizontal, page);
    sliderSize->setRange(2, 70);
    sliderSize->setValue(5);
    connect(sliderSize, &QSlider::valueChanged, [this](int val) {
        emit brushSizeSelected(val);
    });

    layout->addWidget(sliderSize);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}

QWidget *DrawingToolboxDock::createPalettePage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);
    layout->setSpacing(8);

    struct PaletteEntry { QString name; QStringList colors; };
    QList<PaletteEntry> collections = {
        {"Cozy Café", {"#2c211a", "#4d3a2e", "#8b6d5c", "#c68b59", "#dda15e", "#faf5ec"}},
        {"Mossy Forest", {"#1c291e", "#3a5a40", "#588157", "#819b7a", "#a3b18a", "#dad7cd"}},
        {"Sweet Macaron", {"#e07a5f", "#f4a261", "#e76f51", "#f7cad0", "#ffb3c6", "#ffffff"}},
        {"Midnight Sky", {"#0f172a", "#1e293b", "#3d5a80", "#847596", "#9d4edd", "#f3f5fa"}}
    };

    for (const auto &col : collections) {
        QLabel *lbl = new QLabel(col.name, page);
        Sketch::styleSection(lbl);
        layout->addWidget(lbl);

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(6);
        for (const auto &hex : col.colors) {
            QPushButton *swatch = new QPushButton(page);
            swatch->setCheckable(true);
            swatch->setFixedSize(28, 28);
            swatch->setProperty("role", "swatch");
            swatch->setProperty("swatch", QColor(hex));
            m_colorSwatches.append(swatch);
            connect(swatch, &QPushButton::clicked, [this, hex, swatch]() {
                for (auto *s : m_colorSwatches) s->setChecked(s == swatch);
                m_currentBrushColor = QColor(hex);
                emit brushColorSelected(m_currentBrushColor);
            });
            row->addWidget(swatch);
        }
        layout->addLayout(row);
    }

    QPushButton *btnPick = new QPushButton("Pick Custom Color", page);
    connect(btnPick, &QPushButton::clicked, [this]() {
        QColor col = QColorDialog::getColor(m_currentBrushColor, this, "Select Color");
        if (col.isValid()) {
            m_currentBrushColor = col;
            for (auto *s : m_colorSwatches) s->setChecked(false);
            emit brushColorSelected(col);
        }
    });
    layout->addWidget(btnPick);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}

QWidget *DrawingToolboxDock::createShapesPage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);
    layout->setSpacing(8);

    QLabel *lblBubbles = new QLabel("SPEECH BUBBLES", page);
    Sketch::styleSection(lblBubbles);
    layout->addWidget(lblBubbles);

    QHBoxLayout *bubbleRow = new QHBoxLayout();
    bubbleRow->setSpacing(6);
    struct BubbleEntry { QString id; QString label; };
    const QList<BubbleEntry> bubbles = {{"speech", "Speech"}, {"thought", "Thought"}, {"shout", "SHOUT!"}};
    for (const auto &b : bubbles) {
        QPushButton *btn = new QPushButton(b.label, page);
        btn->setToolTip("Adds a bubble and lets you type straight into it. Right-click it to flip the tail.");
        btn->setProperty("wash", QColor("#ffffff"));
        connect(btn, &QPushButton::clicked, [this, b]() {
            emit shapeAdded(b.id, m_shapeFill, m_shapeStroke, m_shapeStrokeWidth);
        });
        bubbleRow->addWidget(btn);
    }
    layout->addLayout(bubbleRow);

    QLabel *lblShapes = new QLabel("STORY SHAPES", page);
    Sketch::styleSection(lblShapes);
    layout->addWidget(lblShapes);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(6);
    QStringList shapes = {"star", "heart", "cloud", "moon", "flower", "circle", "rect"};

    int r = 0, c = 0;
    for (const auto &s : shapes) {
        QString label = s == "rect" ? QString("Box") : s;
        label[0] = label[0].toUpper();
        QPushButton *btn = new QPushButton(label, page);
        btn->setFont(Sketch::headingFont(16));
        connect(btn, &QPushButton::clicked, [this, s]() {
            emit shapeAdded(s, m_shapeFill, m_shapeStroke, m_shapeStrokeWidth);
        });
        grid->addWidget(btn, r, c);
        if (++c > 2) { c = 0; ++r; }
    }
    layout->addLayout(grid);

    QLabel *lblFill = new QLabel("FILL PIGMENT", page);
    Sketch::styleSection(lblFill);
    layout->addWidget(lblFill);

    QHBoxLayout *fillRow = new QHBoxLayout();
    fillRow->setSpacing(6);
    QStringList fillPresets = {"#dda15e", "#c68b59", "#e07a5f", "#819b7a", "#3d5a80", "#faf5ec"};
    for (const auto &hex : fillPresets) {
        QPushButton *swatch = new QPushButton(page);
        swatch->setCheckable(true);
        swatch->setFixedSize(26, 26);
        swatch->setProperty("role", "swatch");
        swatch->setProperty("swatch", QColor(hex));
        m_fillSwatches.append(swatch);
        connect(swatch, &QPushButton::clicked, [this, hex, swatch]() {
            for (auto *f : m_fillSwatches) f->setChecked(f == swatch);
            m_shapeFill = QColor(hex);
        });
        fillRow->addWidget(swatch);
        if (hex == "#dda15e") swatch->setChecked(true);
    }
    layout->addLayout(fillRow);

    layout->addStretch();
    scroll->setWidget(page);
    return scroll;
}

QWidget *DrawingToolboxDock::createStickersPage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);

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
    grid->setSpacing(8);
    int r = 0, c = 0;
    for (const auto &stk : stickers) {
        QPushButton *btn = new QPushButton(stk.name, page);
        btn->setProperty("wash", QColor(stk.fill).lighter(150));
        connect(btn, &QPushButton::clicked, [this, stk]() {
            emit stickerAdded(stk.id, stk.svg, QColor(stk.fill));
        });
        grid->addWidget(btn, r, c);
        if (++c > 1) { c = 0; ++r; }
    }
    layout->addLayout(grid);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}

QWidget *DrawingToolboxDock::createArrangePage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);
    layout->setSpacing(8);

    QPushButton *btnFront = new QPushButton("Bring to Front", page);
    QPushButton *btnBack = new QPushButton("Send to Back", page);
    QPushButton *btnDup = new QPushButton("Duplicate Item", page);
    QPushButton *btnDel = new QPushButton("Delete Item", page);

    btnDel->setProperty("role", "danger");

    connect(btnFront, &QPushButton::clicked, this, &DrawingToolboxDock::bringForwardRequested);
    connect(btnBack, &QPushButton::clicked, this, &DrawingToolboxDock::sendBackwardRequested);
    connect(btnDup, &QPushButton::clicked, this, &DrawingToolboxDock::duplicateRequested);
    connect(btnDel, &QPushButton::clicked, this, &DrawingToolboxDock::deleteRequested);

    layout->addWidget(btnFront);
    layout->addWidget(btnBack);
    layout->addWidget(btnDup);
    layout->addWidget(btnDel);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}
QWidget *DrawingToolboxDock::createTextPage() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *page = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(4, 4, 10, 4);
    layout->setSpacing(8);


    QLabel *lblHint = new QLabel("Click on a page to start writing. Click a text box to edit it, or use the Select tool to move it.", page);
    lblHint->setWordWrap(true);
    Sketch::styleHint(lblHint);
    layout->addWidget(lblHint);

    QLabel *lblFont = new QLabel("STORY FONT", page);
    Sketch::styleSection(lblFont);
    layout->addWidget(lblFont);

    QListWidget *fontList = new QListWidget(page);
    fontList->setMinimumHeight(260);
    for (const auto &font : FONTS) {
        auto *item = new QListWidgetItem(font.name, fontList);
        QFont preview(font.name);
        preview.setPixelSize(20);
        item->setFont(preview);
        item->setToolTip(font.category);
        if (font.name == "Patrick Hand") fontList->setCurrentItem(item);
    }
    connect(fontList, &QListWidget::currentTextChanged, this, [this](const QString &family) {
        if (!family.isEmpty()) emit textFontSelected(family);
    });
    layout->addWidget(fontList);

    QLabel *lblSize = new QLabel("TEXT SIZE", page);
    Sketch::styleSection(lblSize);
    layout->addWidget(lblSize);

    QHBoxLayout *sizeRow = new QHBoxLayout();
    QSlider *sliderSize = new QSlider(Qt::Horizontal, page);
    sliderSize->setRange(12, 96);
    sliderSize->setValue(28);
    QLabel *lblSizeValue = new QLabel("28", page);
    lblSizeValue->setFixedWidth(28);
    connect(sliderSize, &QSlider::valueChanged, this, [this, lblSizeValue](int val) {
        lblSizeValue->setText(QString::number(val));
        emit textSizeSelected(val);
    });
    sizeRow->addWidget(sliderSize, 1);
    sizeRow->addWidget(lblSizeValue);
    layout->addLayout(sizeRow);

    QLabel *lblAlign = new QLabel("ALIGNMENT", page);
    Sketch::styleSection(lblAlign);
    layout->addWidget(lblAlign);

    QHBoxLayout *alignRow = new QHBoxLayout();
    alignRow->setSpacing(6);
    QButtonGroup *alignGroup = new QButtonGroup(page);
    alignGroup->setExclusive(true);
    struct AlignEntry { QString label; Qt::Alignment align; };
    const QList<AlignEntry> aligns = {
        {"Left", Qt::AlignLeft},
        {"Center", Qt::AlignHCenter},
        {"Right", Qt::AlignRight}
    };
    for (const auto &a : aligns) {
        QPushButton *btn = new QPushButton(a.label, page);
        btn->setCheckable(true);
        btn->setProperty("role", "chip");
        alignGroup->addButton(btn);
        connect(btn, &QPushButton::clicked, this, [this, a]() {
            emit textAlignmentSelected(a.align);
        });
        alignRow->addWidget(btn);
        if (a.align == Qt::AlignLeft) btn->setChecked(true);
    }
    layout->addLayout(alignRow);

    QLabel *lblEffect = new QLabel("LETTER STYLE", page);
    Sketch::styleSection(lblEffect);
    layout->addWidget(lblEffect);

    QGridLayout *effectGrid = new QGridLayout();
    effectGrid->setSpacing(6);
    QButtonGroup *effectGroup = new QButtonGroup(page);
    effectGroup->setExclusive(true);
    struct EffectEntry { QString id; QString label; QString tip; };
    const QList<EffectEntry> effects = {
        {"none", "Plain", "Ordinary neat letters"},
        {"wobbly", "Wobbly", "Hand-lettered, every letter a little tipsy"},
        {"bouncy", "Bouncy", "Letters bob up and down like a song"},
        {"shouty", "SHOUTY", "Big and small letters tumbling about, for noisy words"},
        {"rainbow", "Rainbow", "Every letter a different colour"},
    };
    for (int i = 0; i < effects.size(); ++i) {
        const auto e = effects[i];
        QPushButton *btn = new QPushButton(e.label, page);
        btn->setCheckable(true);
        btn->setToolTip(e.tip + ". Shows once you click away from the text.");
        btn->setProperty("role", "chip");
        effectGroup->addButton(btn);
        connect(btn, &QPushButton::clicked, this, [this, e]() { emit textEffectSelected(e.id); });
        effectGrid->addWidget(btn, i / 3, i % 3);
        if (e.id == "none") btn->setChecked(true);
    }
    QPushButton *btnShuffle = new QPushButton("Shuffle", page);
    btnShuffle->setToolTip("Give the selected lettering a fresh jiggle");
    connect(btnShuffle, &QPushButton::clicked, this, &DrawingToolboxDock::textShuffleRequested);
    effectGrid->addWidget(btnShuffle, 1, 2);
    layout->addLayout(effectGrid);

    QLabel *lblColor = new QLabel("INK COLOUR", page);
    Sketch::styleSection(lblColor);
    layout->addWidget(lblColor);

    QHBoxLayout *colorRow = new QHBoxLayout();
    colorRow->setSpacing(6);
    const QStringList textColors = {"#2c211a", "#3d5a80", "#e07a5f", "#606c38", "#847596", "#b48448", "#ffffff"};
    for (const auto &hex : textColors) {
        QPushButton *swatch = new QPushButton(page);
        swatch->setCheckable(true);
        swatch->setFixedSize(26, 26);
        swatch->setProperty("role", "swatch");
        swatch->setProperty("swatch", QColor(hex));
        m_textSwatches.append(swatch);
        connect(swatch, &QPushButton::clicked, this, [this, hex, swatch]() {
            for (auto *s : m_textSwatches) s->setChecked(s == swatch);
            m_currentTextColor = QColor(hex);
            emit textColorSelected(m_currentTextColor);
        });
        colorRow->addWidget(swatch);
    }
    colorRow->addStretch();
    layout->addLayout(colorRow);

    QPushButton *btnPick = new QPushButton("Pick Custom Colour", page);
    connect(btnPick, &QPushButton::clicked, this, [this]() {
        QColor col = QColorDialog::getColor(m_currentTextColor, this, "Text Colour");
        if (col.isValid()) {
            m_currentTextColor = col;
            for (auto *s : m_textSwatches) s->setChecked(false);
            emit textColorSelected(col);
        }
    });
    layout->addWidget(btnPick);
    layout->addStretch();

    scroll->setWidget(page);
    return scroll;
}
