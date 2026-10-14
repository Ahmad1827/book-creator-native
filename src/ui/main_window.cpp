#include "main_window.h"
#include "new_book_dialog.h"
#include "pdf_exporter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QCloseEvent>
#include <QAction>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_stack(new QStackedWidget(this)),
      m_bookshelfView(new BookshelfView(this)),
      m_editorWidget(new QWidget(this)),
      m_canvasView(new BookCanvasView(this)),
      m_toolboxDock(new DrawingToolboxDock(this)),
      m_layersDock(new LayersDock(this)) {

    setWindowTitle("book-creator-native");
    resize(1440, 900);

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    m_autosaveTimer->setInterval(1500);
    connect(m_autosaveTimer, &QTimer::timeout, this, [this]() {
        // Saving ends text editing, so wait until she has finished typing.
        if (isEditorOpen() && m_canvasView->isEditingText()) {
            m_autosaveTimer->start();
            return;
        }
        saveProjectsToDisk();
    });

    setupUi();
    loadProjectsFromDisk();

    connect(m_canvasView, &BookCanvasView::contentChanged, this, &MainWindow::scheduleAutosave);

    QAction *actUndo = new QAction("Undo", this);
    actUndo->setShortcuts({QKeySequence::Undo});
    connect(actUndo, &QAction::triggered, this, [this]() {
        if (isEditorOpen()) m_canvasView->undo();
    });
    addAction(actUndo);

    QAction *actRedo = new QAction("Redo", this);
    actRedo->setShortcuts({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), QKeySequence(Qt::CTRL | Qt::Key_Y)});
    connect(actRedo, &QAction::triggered, this, [this]() {
        if (isEditorOpen()) m_canvasView->redo();
    });
    addAction(actRedo);

    QAction *actSave = new QAction("Save", this);
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, [this]() {
        if (isEditorOpen()) saveProjectsToDisk();
    });
    addAction(actSave);

    QUndoGroup *undoGroup = m_canvasView->undoGroup();
    connect(undoGroup, &QUndoGroup::canUndoChanged, m_btnUndo, &QPushButton::setEnabled);
    connect(undoGroup, &QUndoGroup::canRedoChanged, m_btnRedo, &QPushButton::setEnabled);
    connect(undoGroup, &QUndoGroup::undoTextChanged, this, [this](const QString &text) {
        m_btnUndo->setToolTip(text.isEmpty() ? "Undo (Ctrl+Z)" : QString("Undo %1 (Ctrl+Z)").arg(text.toLower()));
    });
    connect(undoGroup, &QUndoGroup::redoTextChanged, this, [this](const QString &text) {
        m_btnRedo->setToolTip(text.isEmpty() ? "Redo (Ctrl+Shift+Z)" : QString("Redo %1 (Ctrl+Shift+Z)").arg(text.toLower()));
    });

    connect(m_bookshelfView, &BookshelfView::openProjectRequested, this, &MainWindow::openProject);

    connect(m_bookshelfView, &BookshelfView::createProjectRequested, [this]() {
        NewBookDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            BookProject newP = dlg.createProject();
            m_projects.prepend(newP);
            saveProjectsToDisk();
            m_bookshelfView->setProjects(m_projects);
            openProject(newP);
        }
    });

    connect(m_bookshelfView, &BookshelfView::loadProjectRequested, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Load Project", "", "Book Files (*.json)");
        if (path.isEmpty()) return;

        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            BookProject loaded = BookProject::fromJson(doc.object());
            m_projects.prepend(loaded);
            saveProjectsToDisk();
            m_bookshelfView->setProjects(m_projects);
            openProject(loaded);
        }
    });

    connect(m_bookshelfView, &BookshelfView::deleteProjectRequested, [this](const QString &id) {
        if (QMessageBox::question(this, "Delete Notebook", "Delete this storybook notebook?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            for (int i = 0; i < m_projects.size(); ++i) {
                if (m_projects[i].id == id) {
                    m_projects.removeAt(i);
                    break;
                }
            }
            saveProjectsToDisk();
            m_bookshelfView->setProjects(m_projects);
        }
    });

    connect(m_bookshelfView, &BookshelfView::duplicateProjectRequested, [this](const BookProject &proj) {
        BookProject copy = proj;
        copy.id = QString("book-%1").arg(QDateTime::currentMSecsSinceEpoch());
        copy.title = QString("%1 (Copy)").arg(proj.title);
        m_projects.prepend(copy);
        saveProjectsToDisk();
        m_bookshelfView->setProjects(m_projects);
    });

    connect(m_bookshelfView, &BookshelfView::exportJsonRequested, [this](const BookProject &proj) {
        QString safeTitle = proj.title;
        safeTitle.replace(" ", "_");
        QString path = QFileDialog::getSaveFileName(this, "Export Project JSON", QString("%1.lofibook.json").arg(safeTitle), "JSON Files (*.json)");
        if (path.isEmpty()) return;

        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(proj.toJson());
            file.write(doc.toJson(QJsonDocument::Indented));
        }
    });

    connect(m_toolboxDock, &DrawingToolboxDock::toolSelected, this, [this](const QString &toolId) {
        if (toolId == "select") m_canvasView->setMode(CanvasMode::Select);
        else if (toolId == "pan") m_canvasView->setMode(CanvasMode::Pan);
        else if (toolId == "text") m_canvasView->setMode(CanvasMode::Text);
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
    connect(m_toolboxDock, &DrawingToolboxDock::brushSubtypeSelected, m_canvasView, &BookCanvasView::setBrushSubtype);
    connect(m_toolboxDock, &DrawingToolboxDock::textFontSelected, m_canvasView, &BookCanvasView::setTextFontFamily);
    connect(m_toolboxDock, &DrawingToolboxDock::textSizeSelected, m_canvasView, &BookCanvasView::setTextPixelSize);
    connect(m_toolboxDock, &DrawingToolboxDock::textColorSelected, m_canvasView, &BookCanvasView::setTextColor);
    connect(m_toolboxDock, &DrawingToolboxDock::textAlignmentSelected, m_canvasView, &BookCanvasView::setTextAlignment);
    connect(m_layersDock, &LayersDock::applyTemplateRequested, m_canvasView, &BookCanvasView::applyPageTemplate);

    connect(m_layersDock, &LayersDock::addLayerRequested, [this]() {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        CanvasLayer nl;
        nl.id = QString("layer-%1").arg(spread.layers.size() + 1);
        nl.name = QString("Layer %1").arg(spread.layers.size() + 1);
        spread.layers.prepend(nl);
        spread.activeLayerId = nl.id;
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
        saveProjectsToDisk();
    });

    connect(m_layersDock, &LayersDock::renameLayerRequested, [this](const QString &id, const QString &name) {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        for (auto &l : spread.layers) {
            if (l.id == id) l.name = name;
        }
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
        saveProjectsToDisk();
    });

    connect(m_layersDock, &LayersDock::toggleVisibilityRequested, [this](const QString &id) {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        for (auto &l : spread.layers) {
            if (l.id == id) l.visible = !l.visible;
        }
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
    });

    connect(m_layersDock, &LayersDock::toggleLockRequested, [this](const QString &id) {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        for (auto &l : spread.layers) {
            if (l.id == id) l.locked = !l.locked;
        }
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
    });

    connect(m_layersDock, &LayersDock::reorderLayersRequested, [this](int from, int to) {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        spread.layers.move(from, to);
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
        saveProjectsToDisk();
    });

    connect(m_layersDock, &LayersDock::deleteLayerRequested, [this](const QString &id) {
        auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
        if (spread.layers.size() <= 1) return;
        for (int i = 0; i < spread.layers.size(); ++i) {
            if (spread.layers[i].id == id) {
                spread.layers.removeAt(i);
                break;
            }
        }
        spread.activeLayerId = spread.layers[0].id;
        m_layersDock->setLayers(spread.layers, spread.activeLayerId);
        saveProjectsToDisk();
    });

    connect(m_layersDock, &LayersDock::selectLayerRequested, [this](const QString &id) {
        m_activeProject.spreads[m_currentSpreadIndex].activeLayerId = id;
    });
}

void MainWindow::setupUi() {
    QVBoxLayout *editorLayout = new QVBoxLayout(m_editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    setupEditorTopBar(editorLayout);

    QHBoxLayout *workspaceLayout = new QHBoxLayout();
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(0);

    workspaceLayout->addWidget(m_toolboxDock);
    workspaceLayout->addWidget(m_canvasView, 1);
    workspaceLayout->addWidget(m_layersDock);

    editorLayout->addLayout(workspaceLayout, 1);

    setupEditorBottomBar(editorLayout);

    m_stack->addWidget(m_bookshelfView);
    m_stack->addWidget(m_editorWidget);

    setCentralWidget(m_stack);
}

void MainWindow::setupEditorTopBar(QVBoxLayout *layout) {
    QWidget *topBar = new QWidget(m_editorWidget);
    topBar->setFixedHeight(56);
    topBar->setStyleSheet(
        "QWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #442616, stop:1 #2e180d); border-bottom: 3px solid #1c0e07; }"
        "QLabel { color: #faebd7; font-weight: 800; font-size: 13px; }"
        "QComboBox { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 4px 10px; font-weight: bold; color: #3b2212; }"
        "QPushButton { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 6px 14px; font-weight: bold; color: #3b2212; }"
        "QPushButton:hover { background: #ffffff; }"
        "QPushButton:disabled { background: #6b4a33; color: #a88a6c; border-color: #5a351f; }"
    );

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 16, 0);
    topLayout->setSpacing(12);

    QPushButton *btnShelf = new QPushButton("← Shelf", topBar);
    connect(btnShelf, &QPushButton::clicked, [this]() {
        saveProjectsToDisk();
        m_bookshelfView->setProjects(m_projects);
        m_stack->setCurrentIndex(0);
    });
    topLayout->addWidget(btnShelf);

    m_lblTitle = new QLabel(topBar);
    topLayout->addWidget(m_lblTitle);

    m_comboThemes = new QComboBox(topBar);
    for (const auto &theme : THEMES) {
        m_comboThemes->addItem(theme.name, theme.id);
    }
    connect(m_comboThemes, &QComboBox::currentIndexChanged, [this](int index) {
        QString id = m_comboThemes->itemData(index).toString();
        m_activeProject.themeId = id;
        m_canvasView->setTheme(getThemeById(id));
        saveProjectsToDisk();
    });
    topLayout->addWidget(m_comboThemes);

    topLayout->addStretch();

    m_lblSaveState = new QLabel(topBar);
    m_lblSaveState->setStyleSheet("color: #dfbe87; font-weight: 700; font-size: 11px; border: none; background: transparent;");
    topLayout->addWidget(m_lblSaveState);

    m_btnUndo = new QPushButton("↶ Undo", topBar);
    m_btnUndo->setToolTip("Undo (Ctrl+Z)");
    m_btnUndo->setEnabled(false);
    connect(m_btnUndo, &QPushButton::clicked, m_canvasView, &BookCanvasView::undo);
    topLayout->addWidget(m_btnUndo);

    m_btnRedo = new QPushButton("Redo ↷", topBar);
    m_btnRedo->setToolTip("Redo (Ctrl+Shift+Z)");
    m_btnRedo->setEnabled(false);
    connect(m_btnRedo, &QPushButton::clicked, m_canvasView, &BookCanvasView::redo);
    topLayout->addWidget(m_btnRedo);

    QPushButton *btnResetZoom = new QPushButton("Fit Book", topBar);
    connect(btnResetZoom, &QPushButton::clicked, m_canvasView, &BookCanvasView::fitBookInView);
    topLayout->addWidget(btnResetZoom);

    QPushButton *btnExport = new QPushButton("Print Book (PDF)", topBar);
    btnExport->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); "
        "border: 1.5px solid #8c633f; border-radius: 6px; padding: 6px 16px; font-weight: 800; color: #3b2212;"
    );
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::exportPdf);
    topLayout->addWidget(btnExport);

    layout->addWidget(topBar);
}

void MainWindow::setupEditorBottomBar(QVBoxLayout *layout) {
    QWidget *bottomBar = new QWidget(m_editorWidget);
    bottomBar->setFixedHeight(54);
    bottomBar->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #381f13, stop:1 #25130a); border-top: 3px solid #1c0e07;");

    QHBoxLayout *botLayout = new QHBoxLayout(bottomBar);
    botLayout->setContentsMargins(24, 0, 24, 0);

    QHBoxLayout *turnerLayout = new QHBoxLayout();
    m_btnPrevSpread = new QPushButton("← Prev Spread", bottomBar);
    m_lblPageTracker = new QLabel(bottomBar);
    m_btnNextSpread = new QPushButton("Next Spread →", bottomBar);

    QString btnStyle = "background: #fbf5ea; border: 1.5px solid #8d6b4f; color: #3b2212; padding: 5px 14px; border-radius: 16px; font-size: 11px; font-weight: 800;";
    m_btnPrevSpread->setStyleSheet(btnStyle);
    m_btnNextSpread->setStyleSheet(btnStyle);
    m_lblPageTracker->setStyleSheet("font-size: 12px; color: #dfbe87; font-weight: 700; margin: 0 10px;");

    connect(m_btnPrevSpread, &QPushButton::clicked, [this]() {
        switchToSpread(m_currentSpreadIndex - 1);
    });

    connect(m_btnNextSpread, &QPushButton::clicked, [this]() {
        switchToSpread(m_currentSpreadIndex + 1);
    });

    turnerLayout->addWidget(m_btnPrevSpread);
    turnerLayout->addWidget(m_lblPageTracker);
    turnerLayout->addWidget(m_btnNextSpread);
    botLayout->addLayout(turnerLayout);

    botLayout->addStretch();

    m_chipsContainer = new QWidget(bottomBar);
    m_chipsLayout = new QHBoxLayout(m_chipsContainer);
    m_chipsLayout->setContentsMargins(0, 0, 0, 0);
    m_chipsLayout->setSpacing(6);
    botLayout->addWidget(m_chipsContainer);

    QPushButton *btnAddPages = new QPushButton("+ 2 Pages", bottomBar);
    btnAddPages->setStyleSheet("background: transparent; border: 1.5px dashed #dfbe87; color: #dfbe87; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: 700;");
    connect(btnAddPages, &QPushButton::clicked, [this]() {
        const auto &last = m_activeProject.spreads.last();
        BookSpread ns;
        ns.id = QString("s-%1").arg(m_activeProject.spreads.size() + 1);
        ns.leftPageNum = last.rightPageNum + 1;
        ns.rightPageNum = last.rightPageNum + 2;
        ns.activeLayerId = "layer-1";
        CanvasLayer dl;
        dl.id = "layer-1";
        dl.name = "Artwork Layer";
        ns.layers.append(dl);

        m_activeProject.spreads.append(ns);
        switchToSpread(m_activeProject.spreads.size() - 1);
        saveProjectsToDisk();
    });
    botLayout->addWidget(btnAddPages);

    layout->addWidget(bottomBar);
}

void MainWindow::openProject(const BookProject &proj) {
    m_activeProject = proj;
    if (m_activeProject.spreads.isEmpty()) {
        BookSpread first;
        first.id = "s-1";
        first.activeLayerId = "layer-1";
        CanvasLayer layer;
        layer.id = "layer-1";
        layer.name = "Artwork Layer";
        first.layers.append(layer);
        m_activeProject.spreads.append(first);
    }
    m_currentSpreadIndex = 0;
    m_lblTitle->setText(m_activeProject.title);

    {
        QSignalBlocker blocker(m_comboThemes);
        int themeIndex = m_comboThemes->findData(m_activeProject.themeId);
        m_comboThemes->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    }
    m_canvasView->setTheme(getThemeById(m_activeProject.themeId));

    m_canvasView->clearUndoHistory();
    const auto &firstSpread = m_activeProject.spreads[0];
    m_canvasView->loadSpread(firstSpread.id, firstSpread.canvasData);
    m_lblSaveState->setText("All changes saved");

    m_stack->setCurrentIndex(1);
    updateNavigationState();
    m_canvasView->fitBookInView();
    m_canvasView->setFocus();
}

bool MainWindow::isEditorOpen() const {
    return m_stack->currentIndex() == 1;
}

void MainWindow::commitCanvas() {
    if (!isEditorOpen() || m_activeProject.spreads.isEmpty()) return;
    m_activeProject.spreads[m_currentSpreadIndex].canvasData = m_canvasView->saveSpread();
}

void MainWindow::switchToSpread(int index) {
    if (index < 0 || index >= m_activeProject.spreads.size()) return;
    commitCanvas();
    m_currentSpreadIndex = index;
    const auto &spread = m_activeProject.spreads[index];
    m_canvasView->loadSpread(spread.id, spread.canvasData);
    updateNavigationState();
}

void MainWindow::scheduleAutosave() {
    m_lblSaveState->setText("Unsaved changes…");
    m_autosaveTimer->start();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (isEditorOpen()) saveProjectsToDisk();
    event->accept();
}

void MainWindow::updateNavigationState() {
    const auto &spread = m_activeProject.spreads[m_currentSpreadIndex];
    int totalPages = m_activeProject.spreads.size() * 2;

    m_lblPageTracker->setText(QString("Viewing Pages %1 & %2 of %3")
        .arg(spread.leftPageNum)
        .arg(spread.rightPageNum)
        .arg(totalPages));

    m_btnPrevSpread->setEnabled(m_currentSpreadIndex > 0);
    m_btnNextSpread->setEnabled(m_currentSpreadIndex < m_activeProject.spreads.size() - 1);

    m_layersDock->setLayers(spread.layers, spread.activeLayerId);
    m_layersDock->setPageNumbers(spread.leftPageNum, spread.rightPageNum);

    QLayoutItem *child;
    while ((child = m_chipsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (int i = 0; i < m_activeProject.spreads.size(); ++i) {
        const auto &s = m_activeProject.spreads[i];
        QPushButton *chip = new QPushButton(QString("pp. %1-%2").arg(s.leftPageNum).arg(s.rightPageNum), m_chipsContainer);
        if (i == m_currentSpreadIndex) {
            chip->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); color: #3b2212; border: 1.5px solid #8c633f; font-weight: 800; padding: 4px 12px; border-radius: 12px; font-size: 11px;");
        } else {
            chip->setStyleSheet("background: #3b2012; border: 1.5px solid #5a351f; color: #dfbe87; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: 700;");
        }
        connect(chip, &QPushButton::clicked, [this, i]() {
            switchToSpread(i);
        });
        m_chipsLayout->addWidget(chip);
    }
}

void MainWindow::loadProjectsFromDisk() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    QFile file(configDir + "/projects.json");

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.array();
        m_projects.clear();
        for (const auto &val : arr) {
            m_projects.append(BookProject::fromJson(val.toObject()));
        }
    }

    if (m_projects.isEmpty()) {
        BookProject p1;
        p1.id = "book-default-1";
        p1.title = "The Fox & The Firefly";
        p1.author = "Ahmad";
        p1.themeId = "botanical_meadow";
        p1.createdAt = "09/07/2026";

        BookSpread s1;
        s1.id = "s-1";
        s1.leftPageNum = 1;
        s1.rightPageNum = 2;
        s1.activeLayerId = "layer-1";
        CanvasLayer l1;
        l1.id = "layer-1";
        l1.name = "Artwork Layer";
        s1.layers.append(l1);

        BookSpread s2;
        s2.id = "s-2";
        s2.leftPageNum = 3;
        s2.rightPageNum = 4;
        s2.activeLayerId = "layer-1";
        s2.layers.append(l1);

        p1.spreads.append(s1);
        p1.spreads.append(s2);
        m_projects.append(p1);
        saveProjectsToDisk();
    }

    m_bookshelfView->setProjects(m_projects);
}

void MainWindow::saveProjectsToDisk() {
    m_autosaveTimer->stop();
    commitCanvas();

    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].id == m_activeProject.id) {
            m_projects[i] = m_activeProject;
            break;
        }
    }

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    // QSaveFile writes to a temporary file first, so a crash mid-save can't wipe her books.
    QSaveFile file(configDir + "/projects.json");

    bool saved = false;
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (const auto &p : m_projects) {
            arr.append(p.toJson());
        }
        QJsonDocument doc(arr);
        file.write(doc.toJson(QJsonDocument::Compact));
        saved = file.commit();
    }

    if (isEditorOpen()) {
        m_lblSaveState->setText(saved ? "All changes saved" : "⚠ Could not save!");
    }
}

void MainWindow::exportPdf() {
    QString safeTitle = m_activeProject.title;
    safeTitle.replace(" ", "_");

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Storybook to PDF",
        QString("%1.pdf").arg(safeTitle),
        "PDF Files (*.pdf)"
    );
    if (filePath.isEmpty()) return;

    QMessageBox layoutBox(this);
    layoutBox.setWindowTitle("PDF Layout");
    layoutBox.setText("How should the pages be laid out?");
    layoutBox.setInformativeText("Single pages suit printing and binding. Spreads show facing pages side by side, as on screen.");
    QPushButton *btnSingle = layoutBox.addButton("Single Pages (for printing)", QMessageBox::AcceptRole);
    QPushButton *btnSpreads = layoutBox.addButton("Two-Page Spreads", QMessageBox::AcceptRole);
    layoutBox.addButton(QMessageBox::Cancel);
    layoutBox.setDefaultButton(btnSingle);
    layoutBox.exec();
    if (layoutBox.clickedButton() != btnSingle && layoutBox.clickedButton() != btnSpreads) return;
    const auto layout = layoutBox.clickedButton() == btnSingle ? PdfExporter::Layout::SinglePages : PdfExporter::Layout::Spreads;

    saveProjectsToDisk();

    BookTheme theme = getThemeById(m_activeProject.themeId);
    bool success = PdfExporter::exportProjectToPdf(m_activeProject, theme, layout, filePath);

    if (success) {
        QMessageBox::information(this, "Export Succeeded", QString("Your book was saved as a PDF with all %1 pages.").arg(m_activeProject.spreads.size() * 2));
    } else {
        QMessageBox::critical(this, "Export Failed", "Could not generate PDF export.");
    }
}