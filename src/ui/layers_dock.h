#pragma once

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "types.h"

class LayersDock : public QWidget {
    Q_OBJECT

public:
    explicit LayersDock(QWidget *parent = nullptr);

    void setLayers(const QList<CanvasLayer> &layers, const QString &activeLayerId);
    void setPageNumbers(int leftNum, int rightNum);
    QString activeSide() const;

signals:
    void applyTemplateRequested(const QString &side, const QString &templateType);
    void addLayerRequested();
    void renameLayerRequested(const QString &layerId, const QString &newName);
    void toggleVisibilityRequested(const QString &layerId);
    void toggleLockRequested(const QString &layerId);
    void reorderLayersRequested(int fromIndex, int toIndex);
    void deleteLayerRequested(const QString &layerId);
    void selectLayerRequested(const QString &layerId);
    void activeSideChanged(const QString &side);

private:
    void setupUi();
    QWidget *createTemplatesTab();
    QWidget *createLayersTab();
    void refreshLayersList();

    QStackedWidget *m_tabStack;
    QPushButton *m_tabBtnTemplates;
    QPushButton *m_tabBtnLayers;

    QPushButton *m_btnSideLeft;
    QPushButton *m_btnSideRight;
    QString m_activeSide = "left";

    QListWidget *m_layersListWidget;
    QList<CanvasLayer> m_layers;
    QString m_activeLayerId;
};