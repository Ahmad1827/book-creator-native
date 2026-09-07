#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_canvasView(new BookCanvasView(this)),
      m_toolboxDock(new DrawingToolboxDock(this)) {

    setWindowTitle("book-creator-native");
    resize(1360, 850);

    QWidget *centralContainer = new QWidget(this);
    QHBoxLayout *centralLayout = new QHBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    centralLayout->addWidget(m_toolboxDock);
    centralLayout->addWidget(m_canvasView, 1);
    setCentralWidget(centralContainer);

    setupToolbars();

    connect(m_canvasView, &BookCanvasView::activeSideChanged, this, [this](const QString &side) {
        m_activeSide = side;
    });

    connect(m_toolboxDock, &DrawingToolboxDock::toolSelected, this, [this](const QString &toolId) {
        if (toolId == "select") m_canvasView->setMode(CanvasMode::Select);
        else if (toolId == "pan") m_canvasView->setMode(CanvasMode::Pan);
        else m_canvasView->setMode(CanvasMode::Draw);
    });

    connect(m_toolboxDock, &DrawingToolboxDock::brushColorSelected, m_canvasView, &BookCanvasView::setBrushColor);
    connect(m_toolboxDock, &DrawingToolboxDock::brushSizeSelected, m_canvasView, &BookCanvasView::setBrushSize);
    connect(m_toolboxDock, &DrawingToolboxDock::brushOpacitySelected, m_canvasView, &BookCanvasView::setBrushOpacity);
    connect(m_toolboxDock, &DrawingToolboxDock::shapeAdded, m_canvasView, &BookCanvasView::addStoryShape);
    connect(m_toolboxDock, &DrawingToolboxDock::stickerAdded, m_canvasView, &BookCanvasView::addStorySticker);
    connect(m_toolboxDock, &DrawingToolboxDock::duplicateRequested, m_canvasView, &BookCanvasView::duplicateSelectedItem);
    connect(m_toolboxDock, &DrawingToolboxDock::deleteRequested, m_canvasView, &BookCanvasView::deleteSelectedItem);
    connect(m_toolboxDock, &DrawingToolboxDock::bringForwardRequested, m_canvasView, &BookCanvasView::bringSelectedItemToFront);
    connect(m_toolboxDock, &DrawingToolboxDock::sendBackwardRequested, m_canvasView, &BookCanvasView::sendSelectedItemToBack);
}

void MainWindow::setupToolbars() {
    QToolBar *topBar = addToolBar("Tools");
    topBar->setMovable(false);
    topBar->setStyleSheet(
        "QToolBar { background: #381f13; border-bottom: 2px solid #1c0e07; padding: 4px; gap: 8px; }"
        "QPushButton { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 5px 12px; font-weight: bold; color: #3b2212; }"
        "QPushButton:hover { background: #ffffff; }"
        "QComboBox { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 4px 8px; font-weight: bold; color: #3b2212; }"
    );

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

    QPushButton *btnApplyTemplate = new QPushButton("Apply Layout to Active Page", this);
    connect(btnApplyTemplate, &QPushButton::clicked, [this, comboTemplates]() {
        m_canvasView->applyPageTemplate(m_activeSide, comboTemplates->currentData().toString());
    });

    topBar->addWidget(comboTemplates);
    topBar->addWidget(btnApplyTemplate);
}