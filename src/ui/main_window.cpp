#include "main_window.h"
#include "new_book_dialog.h"
#include "pdf_exporter.h"
#include "page_sorter_dialog.h"
#include "asset_store.h"
#include "app_settings.h"
#include "settings_dialog.h"
#include "export_dialog.h"
#include "sketch.h"
#include <QApplication>
#include <QImageReader>
#include <QUuid>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QCloseEvent>
#include <QAction>
#include <QSignalBlocker>

namespace {

QString newSpreadId() {
    return "s-" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QJsonObject coverText(const QString &text, const BookTheme &theme, int pixelSize, qreal y, int z) {
    QJsonObject obj;
    obj["type"] = "text";
    obj["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    obj["text"] = text;
    obj["font"] = theme.recommendedFont;
    obj["size"] = pixelSize;
    obj["color"] = theme.inkColor.name(QColor::HexArgb);
    obj["align"] = "center";
    obj["width"] = 480;
    obj["x"] = 660;
    obj["y"] = y;
    obj["z"] = z;
    return obj;
}

// Back cover on the left, front cover on the right, like a real book's jacket laid flat.
BookSpread makeCoverSpread(const BookProject &project) {
    const BookTheme &theme = getThemeById(project.themeId);
    BookSpread cover = BookSpread::blank("cover");
    cover.kind = "cover";
    QJsonArray items;
    items.append(coverText(project.title, theme, 56, 110, 1));
    items.append(coverText(QString("by %1").arg(project.author), theme, 26, 500, 2));
    cover.canvasData["version"] = 1;
    cover.canvasData["items"] = items;
    return cover;
}

QStringList assetIdsIn(const BookProject &project) {
    QStringList ids;
    for (const auto &spread : project.spreads) {
        for (const auto &val : spread.canvasData["items"].toArray()) {
            const QString asset = val.toObject()["asset"].toString();
            if (!asset.isEmpty() && !ids.contains(asset)) ids.append(asset);
        }
    }
    return ids;
}

} // namespace

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
    connect(m_bookshelfView, &BookshelfView::settingsRequested, this, &MainWindow::openSettings);
    applySettings();

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
            const QJsonObject assets = doc.object()["assets"].toObject();
            for (auto it = assets.constBegin(); it != assets.constEnd(); ++it) {
                AssetStore::addEncoded(QByteArray::fromBase64(it.value().toString().toLatin1()), QFileInfo(it.key()).suffix());
            }
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

        // Pictures are packed into the file so the book can move to another computer.
        QJsonObject obj = proj.toJson();
        QJsonObject assets;
        for (const QString &id : assetIdsIn(proj)) {
            const QByteArray bytes = AssetStore::bytes(id);
            if (!bytes.isEmpty()) assets[id] = QString::fromLatin1(bytes.toBase64());
        }
        if (!assets.isEmpty()) obj["assets"] = assets;

        QSaveFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            if (!file.commit()) QMessageBox::critical(this, "Save Failed", "Could not write the book file.");
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
    connect(m_toolboxDock, &DrawingToolboxDock::textEffectSelected, m_canvasView, &BookCanvasView::setTextEffect);
    connect(m_toolboxDock, &DrawingToolboxDock::textShuffleRequested, m_canvasView, &BookCanvasView::shuffleTextEffect);
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
    auto *topBar = new SketchPanel(m_editorWidget, Sketch::kPaper, SketchPanel::BottomEdge);
    topBar->setFixedHeight(72);

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(14, 4, 14, 6);
    topLayout->setSpacing(8);

    QPushButton *btnShelf = new QPushButton("← Bookshelf", topBar);
    btnShelf->setProperty("role", "ghost");
    connect(btnShelf, &QPushButton::clicked, [this]() {
        if (m_helperDialog) m_helperDialog->close();
        saveProjectsToDisk();
        m_bookshelfView->setProjects(m_projects);
        m_stack->setCurrentIndex(0);
    });
    topLayout->addWidget(btnShelf);

    m_lblTitle = new QLabel(topBar);
    m_lblTitle->setFont(Sketch::titleFont(26));
    m_lblTitle->setMaximumWidth(380);
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

    QPushButton *btnPicture = new QPushButton("+ Add Picture", topBar);
    btnPicture->setProperty("wash", Sketch::kPink);
    btnPicture->setToolTip("Add a scanned painting or photo to the page you last clicked.\nYou can also drag pictures onto the book, or paste with Ctrl+V.");
    connect(btnPicture, &QPushButton::clicked, this, [this]() {
        QStringList patterns;
        for (const QByteArray &fmt : QImageReader::supportedImageFormats()) patterns << "*." + QString::fromLatin1(fmt);
        const QStringList paths = QFileDialog::getOpenFileNames(this, "Add Pictures", QString(),
            QString("Pictures (%1)").arg(patterns.join(' ')));
        for (const QString &path : paths) {
            QString error;
            if (!m_canvasView->importImageFile(path, &error)) {
                QMessageBox::warning(this, "Add Picture", QString("Couldn't open %1.\n%2").arg(QFileInfo(path).fileName(), error));
            }
        }
        m_canvasView->setFocus();
    });
    topLayout->addWidget(btnPicture);

    m_btnHelper = new QPushButton("Story Helper", topBar);
    m_btnHelper->setProperty("wash", Sketch::kTeal);
    m_btnHelper->setToolTip("Ideas for your words from Claude: rhymes, silly words, what happens next…");
    connect(m_btnHelper, &QPushButton::clicked, this, &MainWindow::openStoryHelper);
    topLayout->addWidget(m_btnHelper);

    m_lblSaveState = Sketch::hint(QString(), topBar);
    m_lblSaveState->setWordWrap(false);
    m_lblSaveState->setMinimumWidth(110);
    m_lblSaveState->setAlignment(Qt::AlignCenter);
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
    btnResetZoom->setProperty("role", "ghost");
    connect(btnResetZoom, &QPushButton::clicked, m_canvasView, &BookCanvasView::fitBookInView);
    topLayout->addWidget(btnResetZoom);

    QPushButton *btnExport = new QPushButton("Print Book", topBar);
    btnExport->setProperty("role", "primary");
    btnExport->setToolTip("Save the whole book as a PDF for printing or sharing");
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::exportPdf);
    topLayout->addWidget(btnExport);

    layout->addWidget(topBar);
}

void MainWindow::setupEditorBottomBar(QVBoxLayout *layout) {
    auto *bottomBar = new SketchPanel(m_editorWidget, Sketch::kPaper, SketchPanel::TopEdge);
    bottomBar->setFixedHeight(66);

    QHBoxLayout *botLayout = new QHBoxLayout(bottomBar);
    botLayout->setContentsMargins(18, 6, 18, 4);
    botLayout->setSpacing(8);

    QHBoxLayout *turnerLayout = new QHBoxLayout();
    turnerLayout->setSpacing(8);
    m_btnPrevSpread = new QPushButton("← Turn back", bottomBar);
    m_lblPageTracker = new QLabel(bottomBar);
    m_lblPageTracker->setFont(Sketch::headingFont(19));
    m_lblPageTracker->setContentsMargins(8, 0, 8, 0);
    m_btnNextSpread = new QPushButton("Turn the page →", bottomBar);
    m_btnPrevSpread->setProperty("role", "chip");
    m_btnNextSpread->setProperty("role", "chip");

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
    m_chipsLayout->setSpacing(4);
    botLayout->addWidget(m_chipsContainer);

    QPushButton *btnAddPages = new QPushButton("+ 2 Pages", bottomBar);
    btnAddPages->setProperty("role", "chip");
    btnAddPages->setProperty("wash", Sketch::kGreen);
    connect(btnAddPages, &QPushButton::clicked, [this]() {
        m_activeProject.spreads.append(BookSpread::blank(newSpreadId()));
        m_activeProject.renumberPages();
        switchToSpread(m_activeProject.spreads.size() - 1);
        saveProjectsToDisk();
    });
    botLayout->addWidget(btnAddPages);

    QPushButton *btnArrange = new QPushButton("☰ Arrange Pages", bottomBar);
    btnArrange->setProperty("wash", Sketch::kLilac);
    connect(btnArrange, &QPushButton::clicked, this, &MainWindow::arrangePages);
    botLayout->addWidget(btnArrange);

    layout->addWidget(bottomBar);
}

void MainWindow::openProject(const BookProject &proj) {
    m_activeProject = proj;
    if (m_activeProject.pageCount() == 0) {
        m_activeProject.spreads.append(BookSpread::blank(newSpreadId()));
    }
    if (!m_activeProject.hasCover()) {
        m_activeProject.spreads.prepend(makeCoverSpread(m_activeProject));
    }
    m_activeProject.renumberPages();
    m_currentSpreadIndex = 1;
    m_lblTitle->setText(m_activeProject.title);

    {
        QSignalBlocker blocker(m_comboThemes);
        int themeIndex = m_comboThemes->findData(m_activeProject.themeId);
        m_comboThemes->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    }
    m_canvasView->setTheme(getThemeById(m_activeProject.themeId));
    m_canvasView->setPageHeight(m_activeProject.pageHeight());

    m_canvasView->clearUndoHistory();
    const auto &firstSpread = m_activeProject.spreads[m_currentSpreadIndex];
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

void MainWindow::arrangePages() {
    commitCanvas();
    const QString currentId = m_activeProject.spreads[m_currentSpreadIndex].id;
    PageSorterDialog dlg(m_activeProject, currentId, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_activeProject.spreads = dlg.spreads();
    m_activeProject.showPageNumbers = dlg.showPageNumbers();
    const bool resized = m_activeProject.formatId != dlg.formatId();
    m_activeProject.formatId = dlg.formatId();
    m_activeProject.renumberPages();
    m_canvasView->setPageHeight(m_activeProject.pageHeight());

    int index = 0;
    for (int i = 0; i < m_activeProject.spreads.size(); ++i) {
        if (m_activeProject.spreads[i].id == dlg.focusSpreadId()) index = i;
    }
    // The spreads were already committed, so load directly instead of committing over them.
    m_currentSpreadIndex = index;
    const auto &spread = m_activeProject.spreads[index];
    m_canvasView->loadSpread(spread.id, spread.canvasData);
    updateNavigationState();
    if (resized) m_canvasView->fitBookInView();
    saveProjectsToDisk();
}

void MainWindow::openSettings() {
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) applySettings();
}

void MainWindow::applySettings() {
    const bool helperOn = AppSettings::helperBackend() == AppSettings::HelperBackend::ClaudeCode;
    m_btnHelper->setVisible(helperOn);
    if (!helperOn && m_helperDialog) m_helperDialog->close();
}

void MainWindow::openStoryHelper() {
    if (!m_helperDialog) {
        m_helperDialog = new StoryHelperDialog(m_canvasView, [this]() {
            commitCanvas();
            return qMakePair(m_activeProject.title, StoryHelper::storyText(m_activeProject));
        }, this);
        m_helperDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_helperDialog, &StoryHelperDialog::settingsRequested, this, &MainWindow::openSettings);
        // Park it beside the book rather than on top of the pages.
        const QPoint topRight = mapToGlobal(QPoint(width() - m_helperDialog->width() - 40, 80));
        m_helperDialog->move(topRight);
    } else {
        m_helperDialog->pullSelectedText();
    }
    m_helperDialog->show();
    m_helperDialog->raise();
    m_helperDialog->activateWindow();
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
    int totalPages = m_activeProject.pageCount();

    if (spread.isCover()) {
        m_lblPageTracker->setText("The Cover");
        m_canvasView->setPageNumbers(0, 0);
    } else {
        m_lblPageTracker->setText(QString("Pages %1 & %2 of %3")
            .arg(spread.leftPageNum)
            .arg(spread.rightPageNum)
            .arg(totalPages));
        const bool numbers = m_activeProject.showPageNumbers;
        m_canvasView->setPageNumbers(numbers ? spread.leftPageNum : 0, numbers ? spread.rightPageNum : 0);
    }

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
        const QString chipText = s.isCover() ? QString("Cover") : QString("pp. %1-%2").arg(s.leftPageNum).arg(s.rightPageNum);
        QPushButton *chip = new QPushButton(chipText, m_chipsContainer);
        chip->setProperty("role", "chip");
        chip->setFont(Sketch::headingFont(16));
        chip->setCheckable(true);
        chip->setChecked(i == m_currentSpreadIndex);
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
    // Commit the open spread so the export and the blurry-picture check see her latest work.
    saveProjectsToDisk();

    ExportDialog options(m_activeProject, this);
    if (options.exec() != QDialog::Accepted) return;

    QString safeTitle = m_activeProject.title;
    safeTitle.replace(" ", "_");
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Storybook to PDF",
        QString("%1.pdf").arg(safeTitle),
        "PDF Files (*.pdf)"
    );
    if (filePath.isEmpty()) return;

    BookTheme theme = getThemeById(m_activeProject.themeId);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool success = PdfExporter::exportProjectToPdf(m_activeProject, theme, options.options(), filePath);
    QApplication::restoreOverrideCursor();

    if (success) {
        QMessageBox::information(this, "Export Succeeded", QString("Your book was saved as a PDF with all %1 pages%2.").arg(m_activeProject.pageCount()).arg(m_activeProject.hasCover() ? " plus the covers" : ""));
    } else {
        QMessageBox::critical(this, "Export Failed", "Could not generate PDF export.");
    }
}
