#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTabletEvent>
#include <QMenu>
#include <QAction>
#include <QPointF>
#include <QPointer>
#include <QHash>
#include <QUndoGroup>
#include <QUndoStack>
#include "types.h"
#include "constants.h"
#include "theme_decors.h"
#include "canvas_items.h"

enum class CanvasMode {
    Select,
    Draw,
    Text,
    Pan
};

class BookCanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit BookCanvasView(QWidget *parent = nullptr);

    void setTheme(const BookTheme &theme);
    void setMode(CanvasMode mode);
    void setBrushSubtype(const QString &subtype);
    void setBrushColor(const QColor &color);
    void setBrushSize(qreal size);
    void setBrushOpacity(qreal opacity);
    void applyPageTemplate(const QString &side, const QString &templateType);

    void setTextFontFamily(const QString &family);
    void setTextPixelSize(int size);
    void setTextColor(const QColor &color);
    void setTextAlignment(Qt::Alignment align);

    void addStoryShape(const QString &shapeType, const QColor &fill, const QColor &stroke, qreal strokeWidth);
    void addStorySticker(const QString &stickerId, const QString &svgPath, const QColor &fill);

    void duplicateSelectedItem();
    void deleteSelectedItem();
    void bringSelectedItemToFront();
    void sendSelectedItemToBack();
    void fitBookInView();

    // Spread persistence: each spread keeps its own artwork and undo history.
    void loadSpread(const QString &spreadId, const QJsonObject &canvasData);
    QJsonObject saveSpread();
    void clearUndoHistory();

    bool isEditingText() const { return editingTextItem() != nullptr; }

    QUndoGroup *undoGroup() const { return m_undoGroup; }
    void undo();
    void redo();

    // Used by undo commands to swap item states in and out of the scene.
    void applyItemStates(const QStringList &ids, const QHash<QString, QJsonObject> &states);

signals:
    void activeSideChanged(const QString &side);
    void selectionChanged(QGraphicsItem *item);
    void contentChanged();

protected:
    bool viewportEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    bool handleTabletEvent(QTabletEvent *event);
    QPointF viewportToScene(const QPointF &viewportPos) const;
    void updateActiveSide(const QPointF &scenePos);

    void beginStroke(const QPointF &scenePos, qreal pressure, bool forceEraser);
    void extendStroke(const QPointF &scenePos, qreal pressure);
    void endStroke();
    void eraseAtPoint(const QPointF &scenePos);

    void handleTextClick(const QPointF &scenePos, QMouseEvent *event);
    StoryTextItem *textItemAt(const QPointF &scenePos) const;
    StoryTextItem *editingTextItem() const;
    void finishTextEditing();
    void onTextEditingFinished(StoryTextItem *item);
    QList<StoryTextItem*> selectedTextItems() const;

    void adoptItem(QGraphicsItem *item);
    void applyInteractionFlags(QGraphicsItem *item);
    void updateCursor();
    qreal nextTopZ() const;
    void recordChange(const QString &label);
    QHash<QString, QJsonObject> captureItemStates() const;
    QGraphicsItem *findItemById(const QString &id) const;

    void showContextMenu(const QPoint &globalPos, QGraphicsItem *item);
    void duplicateItem(QGraphicsItem *item);
    void deleteItem(QGraphicsItem *item);
    void bringItemToFront(QGraphicsItem *item);
    void sendItemToBack(QGraphicsItem *item);
    void toggleLockItem(QGraphicsItem *item);

    QGraphicsScene *m_scene;
    BookTheme m_theme;
    CanvasMode m_mode = CanvasMode::Draw;
    QString m_brushSubtype = "ink";
    QColor m_brushColor = QColor("#1d291e");
    qreal m_brushSize = 4.0;
    qreal m_brushOpacity = 1.0;

    QString m_textFamily = "Patrick Hand";
    int m_textPixelSize = 28;
    QColor m_textColor = QColor("#1d291e");
    Qt::Alignment m_textAlign = Qt::AlignLeft;

    bool m_isDrawing = false;
    bool m_isErasing = false;
    bool m_tabletStrokeActive = false;
    InkStrokeItem *m_currentStroke = nullptr;
    QPointF m_smoothedPos;
    qreal m_smoothedPressure = 1.0;
    QPointF m_lastRawPos;
    QString m_activeSide = "left";

    bool m_isMiddlePanning = false;
    bool m_isSpacePanning = false;
    bool m_spacePressed = false;
    QPoint m_lastPanPoint;
    bool m_initialFitDone = false;

    QUndoGroup *m_undoGroup;
    QHash<QString, QUndoStack*> m_undoStacks;
    QHash<QString, QJsonObject> m_itemStates;
    bool m_applyingUndo = false;
};
