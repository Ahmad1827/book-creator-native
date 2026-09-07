#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include "book_canvas_view.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupToolbars();
    BookCanvasView *m_canvasView;
    QString m_activeSide = "left";
};