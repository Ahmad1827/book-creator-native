#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTabletEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
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
    void setTextEffect(const QString &effect);
    // Re-rolls the jiggle of the selected lettering.
    void shuffleTextEffect();

    void addStoryShape(const QString &shapeType, const QColor &fill, const QColor &stroke, qreal strokeWidth);
    void addStorySticker(const QString &stickerId, const QString &svgPath, const QColor &fill);

    // Pictures are fitted into the page layout's picture frame, or the page if there is none.
    bool importImageFile(const QString &path, QString *error = nullptr);
    bool importImage(const QImage &image);
    void setPageNumbers(int leftNum, int rightNum);
    // Page proportions come from the book's print size; pages are always 600 units wide.
    void setPageHeight(qreal pageHeight);
    qreal pageHeight() const { return m_pageHeight; }

    // Story Helper hooks: the text box she selected (or last worked on).
    QString selectedStoryText() const;
    bool replaceSelectedStoryText(const QString &text);
    void addStoryTextBox(const QString &text);

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
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
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
    StoryTextItem *helperTargetText() const;
    void finishTextEditing();
    void onTextEditingFinished(StoryTextItem *item);
    QList<StoryTextItem*> selectedTextItems() const;

    enum class HandleDrag { None, Resize, Rotate };
    struct HandleGeometry {
        QPolygonF outline;
        QPointF center;
        QPointF resize;
        QPointF rotateBase;
        QPointF rotate;
    };
    QGraphicsItem *handleTarget() const;
    HandleGeometry handleGeometry(QGraphicsItem *item) const;
    HandleDrag handleAt(const QPointF &viewportPos) const;
    void beginHandleDrag(HandleDrag drag, const QPointF &scenePos);
    void updateHandleDrag(const QPointF &scenePos, Qt::KeyboardModifiers modifiers);

    bool placeImage(const QString &assetId, const QPointF *dropPos);
    bool pasteFromMime(const QMimeData *mime, const QPointF *dropPos);
    QRectF artFrameFor(const QString &side, const QPointF *point) const;
    QRectF pageInnerRect(const QString &side) const;
    void fitItemInto(QGraphicsItem *item, const QRectF &frame, bool cover);
    static void centerTransformOrigin(QGraphicsItem *item);

    void addBubble(const QString &kind);
    void applyNewTextStyle(StoryTextItem *text) const;

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
    QString m_textEffect = "none";

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
    // Keep the whole book in view as panels open and close, until she zooms or pans herself.
    bool m_autoFit = true;

    QPointer<StoryTextItem> m_lastTextItem;
    qreal m_pageHeight = 650.0;
    int m_leftPageNum = 0;
    int m_rightPageNum = 0;

    HandleDrag m_handleDrag = HandleDrag::None;
    QGraphicsItem *m_handleItem = nullptr;
    QPointF m_dragCenter;
    qreal m_dragStartScale = 1.0;
    qreal m_dragStartDistance = 1.0;
    qreal m_dragStartRotation = 0.0;
    qreal m_dragStartAngle = 0.0;

    QUndoGroup *m_undoGroup;
    QHash<QString, QUndoStack*> m_undoStacks;
    QHash<QString, QJsonObject> m_itemStates;
    bool m_applyingUndo = false;
};
