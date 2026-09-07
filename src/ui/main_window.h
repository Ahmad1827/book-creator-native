#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "types.h"
#include "constants.h"
#include "book_canvas_view.h"
#include "drawing_toolbox_dock.h"
#include "layers_dock.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupLayout();
    void setupTopBar(QVBoxLayout *rootLayout);
    void setupBottomBar(QVBoxLayout *rootLayout);
    void updateNavigationState();
    void exportPdf();

    BookProject m_project;
    int m_currentSpreadIndex = 0;

    BookCanvasView *m_canvasView;
    DrawingToolboxDock *m_toolboxDock;
    LayersDock *m_layersDock;

    QLabel *m_lblPageTracker;
    QPushButton *m_btnPrevSpread;
    QPushButton *m_btnNextSpread;
    QHBoxLayout *m_chipsLayout;
    QWidget *m_chipsContainer;
};