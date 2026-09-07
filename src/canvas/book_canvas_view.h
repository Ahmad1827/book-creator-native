#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include "types.h"
#include "constants.h"

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
    void applyPageTemplate(const QString &side, const QString &templateType);

signals:
    void activeSideChanged(const QString &side);
    void selectionChanged(QGraphicsItem *item);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

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
    bool m_isDrawing = false;
    QPainterPath m_currentPath;
    QGraphicsPathItem *m_currentPathItem = nullptr;
};