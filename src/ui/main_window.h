#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include "types.h"
#include "constants.h"
#include "book_canvas_view.h"
#include "drawing_toolbox_dock.h"
#include "layers_dock.h"
#include "bookshelf_view.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupEditorTopBar(QVBoxLayout *layout);
    void setupEditorBottomBar(QVBoxLayout *layout);
    void updateNavigationState();
    void openProject(const BookProject &proj);
    void switchToSpread(int index);
    void commitCanvas();
    bool isEditorOpen() const;
    void scheduleAutosave();
    void arrangePages();
    void loadProjectsFromDisk();
    void saveProjectsToDisk();
    void exportPdf();

    QList<BookProject> m_projects;
    BookProject m_activeProject;
    int m_currentSpreadIndex = 0;

    QStackedWidget *m_stack;
    BookshelfView *m_bookshelfView;
    QWidget *m_editorWidget;

    BookCanvasView *m_canvasView;
    DrawingToolboxDock *m_toolboxDock;
    LayersDock *m_layersDock;

    QLabel *m_lblPageTracker;
    QLabel *m_lblTitle;
    QPushButton *m_btnPrevSpread;
    QPushButton *m_btnNextSpread;
    QHBoxLayout *m_chipsLayout;
    QWidget *m_chipsContainer;
    QComboBox *m_comboThemes;
    QLabel *m_lblSaveState;
    QPushButton *m_btnUndo;
    QPushButton *m_btnRedo;
    QTimer *m_autosaveTimer;
};