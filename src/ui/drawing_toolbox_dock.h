#pragma once

#include <QWidget>
#include <QFrame>
#include <QStackedWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QColorDialog>
#include <QIcon>
#include "types.h"
#include "constants.h"

class DrawingToolboxDock : public QWidget {
    Q_OBJECT

public:
    explicit DrawingToolboxDock(QWidget *parent = nullptr);

signals:
    void toolSelected(const QString &toolId);
    void brushSubtypeSelected(const QString &subtype);
    void brushColorSelected(const QColor &color);
    void brushSizeSelected(qreal size);
    void brushOpacitySelected(qreal opacity);
    void shapeAdded(const QString &shapeType, const QColor &fill, const QColor &stroke, qreal strokeWidth);
    void stickerAdded(const QString &stickerId, const QString &svgPath, const QColor &fill);
    void bringForwardRequested();
    void sendBackwardRequested();
    void duplicateRequested();
    void deleteRequested();

private:
    void setupUi();
    void buildIconRail();
    void buildDrawerPages();
    QIcon createSvgIcon(const QString &pathD, const QString &fillColor = "#5c351f");

    QWidget *createPencilPage();
    QWidget *createBrushPage();
    QWidget *createPalettePage();
    QWidget *createShapesPage();
    QWidget *createStickersPage();
    QWidget *createArrangePage();

    QWidget *m_iconRail;
    QFrame *m_drawer;
    QStackedWidget *m_stackedDrawer;
    QLabel *m_drawerTitle;

    QColor m_currentBrushColor = QColor("#1d291e");
    qreal m_currentBrushSize = 4.0;
    qreal m_currentBrushOpacity = 1.0;

    QColor m_shapeFill = QColor("#dda15e");
    QColor m_shapeStroke = QColor("#2c211a");
    qreal m_shapeStrokeWidth = 2.0;

    bool m_isDrawerOpen = false;
    QButtonGroup *m_railGroup;
};