#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include "types.h"
#include "constants.h"
#include "theme_decors.h"

enum class CanvasMode {
    Select,
    Draw,
    Pan
};

class BookCanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit BookCanvasView(QWidget *parent = nullptr);

    void setTheme(const BookTheme &theme);
    void setMode(CanvasMode mode);
    void setBrushColor(const QColor &color);
    void setBrushSize(qreal size);
    void setBrushOpacity(qreal opacity);
    void applyPageTemplate(const QString &side, const QString &templateType);

    void addStoryShape(const QString &shapeType, const QColor &fill, const QColor &stroke, qreal strokeWidth);
    void addStorySticker(const QString &stickerId, const QString &svgPath, const QColor &fill);

    void duplicateSelectedItem();
    void deleteSelectedItem();
    void bringSelectedItemToFront();
    void sendSelectedItemToBack();
    void fitBookInView();

signals:
    void activeSideChanged(const QString &side);
    void selectionChanged(QGraphicsItem *item);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void showContextMenu(const QPoint &globalPos, QGraphicsItem *item);
    void duplicateItem(QGraphicsItem *item);
    void deleteItem(QGraphicsItem *item);
    void bringItemToFront(QGraphicsItem *item);
    void sendItemToBack(QGraphicsItem *item);
    void toggleLockItem(QGraphicsItem *item);

    QGraphicsScene *m_scene;
    BookTheme m_theme;
    CanvasMode m_mode = CanvasMode::Draw;
    QColor m_brushColor = QColor("#1d291e");
    qreal m_brushSize = 4.0;
    qreal m_brushOpacity = 1.0;

    bool m_isDrawing = false;
    QPainterPath m_currentPath;
    QGraphicsPathItem *m_currentPathItem = nullptr;
    QString m_activeSide = "left";

    bool m_isMiddlePanning = false;
    bool m_isSpacePanning = false;
    bool m_spacePressed = false;
    QPoint m_lastPanPoint;
    bool m_initialFitDone = false;
};