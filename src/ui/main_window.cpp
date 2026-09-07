#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_canvasView(new BookCanvasView(this)) {

    setWindowTitle("book-creator-native");
    resize(1280, 800);
    setCentralWidget(m_canvasView);

    setupToolbars();

    connect(m_canvasView, &BookCanvasView::activeSideChanged, this, [this](const QString &side) {
        m_activeSide = side;
    });
}

void MainWindow::setupToolbars() {
    QToolBar *topBar = addToolBar("Tools");
    topBar->setMovable(false);
    topBar->setStyleSheet(
        "QToolBar { background: #381f13; border-bottom: 2px solid #1c0e07; padding: 4px; gap: 8px; }"
        "QPushButton { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 5px 12px; font-weight: bold; color: #3b2212; }"
        "QPushButton:hover { background: #ffffff; }"
        "QComboBox { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #3b2212; }"
        "QSpinBox { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #3b2212; }"
    );

    QPushButton *btnDraw = new QPushButton("Draw Mode", this);
    QPushButton *btnSelect = new QPushButton("Select Mode", this);
    QPushButton *btnPan = new QPushButton("Pan Mode", this);

    connect(btnDraw, &QPushButton::clicked, [this]() { m_canvasView->setMode(CanvasMode::Draw); });
    connect(btnSelect, &QPushButton::clicked, [this]() { m_canvasView->setMode(CanvasMode::Select); });
    connect(btnPan, &QPushButton::clicked, [this]() { m_canvasView->setMode(CanvasMode::Pan); });

    topBar->addWidget(btnDraw);
    topBar->addWidget(btnSelect);
    topBar->addWidget(btnPan);

    QSpinBox *spinBrushSize = new QSpinBox(this);
    spinBrushSize->setRange(1, 60);
    spinBrushSize->setValue(4);
    connect(spinBrushSize, &QSpinBox::valueChanged, [this](int val) {
        m_canvasView->setBrushSize(val);
    });
    topBar->addWidget(spinBrushSize);

    QComboBox *comboThemes = new QComboBox(this);
    for (const auto &theme : THEMES) {
        comboThemes->addItem(theme.name, theme.id);
    }
    connect(comboThemes, &QComboBox::currentIndexChanged, [this, comboThemes](int index) {
        QString id = comboThemes->itemData(index).toString();
        m_canvasView->setTheme(getThemeById(id));
    });
    topBar->addWidget(comboThemes);

    QComboBox *comboTemplates = new QComboBox(this);
    comboTemplates->addItem("Template: Top Art / Bottom Text", "top_art_bottom_text");
    comboTemplates->addItem("Template: Bottom Art / Top Text", "bottom_art_top_text");
    comboTemplates->addItem("Template: Storyboard (2 Panels)", "storyboard_panels");
    comboTemplates->addItem("Template: Character Spot & Card", "spot_rhyme");
    comboTemplates->addItem("Template: Vertical Split (50/50)", "split_vertical");

    QPushButton *btnApplyTemplate = new QPushButton("Apply to Active Page", this);
    connect(btnApplyTemplate, &QPushButton::clicked, [this, comboTemplates]() {
        m_canvasView->applyPageTemplate(m_activeSide, comboTemplates->currentData().toString());
    });

    topBar->addWidget(comboTemplates);
    topBar->addWidget(btnApplyTemplate);
}