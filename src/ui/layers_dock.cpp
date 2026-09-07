#include "layers_dock.h"
#include <QInputDialog>

LayersDock::LayersDock(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void LayersDock::setupUi() {
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_handleBtn = new QPushButton(this);
    m_handleBtn->setFixedWidth(36);
    m_handleBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_handleBtn->setText("▶\n\nL\nA\nY\nE\nR\nS");
    m_handleBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2b170c, stop:1 #381f13); "
        "border-left: 2px solid #1c0e07; border-top: none; border-bottom: none; border-right: none; "
        "color: #dfbe87; font-weight: 800; font-size: 10px; line-height: 14px; padding: 8px 0; } "
        "QPushButton:hover { color: #ffffff; background: #442617; }"
    );
    m_handleBtn->setToolTip("Toggle Layout & Layers Panel");
    connect(m_handleBtn, &QPushButton::clicked, this, &LayersDock::toggleOpen);
    rootLayout->addWidget(m_handleBtn);

    m_contentFrame = new QFrame(this);
    m_contentFrame->setFixedWidth(280);
    m_contentFrame->setStyleSheet(
        "QFrame { background: #fbf5ea; border-left: 3px solid #8d6b4f; color: #3b2212; }"
        "QListWidget { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 8px; }"
        "QListWidget::item { border-bottom: 1px solid #ebd9c3; padding: 6px; }"
        "QListWidget::item:selected { background: #ecd9bf; }"
    );

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(10);

    QHBoxLayout *topRow = new QHBoxLayout();
    QHBoxLayout *tabSwitcher = new QHBoxLayout();
    tabSwitcher->setSpacing(4);

    m_tabBtnTemplates = new QPushButton("Page Templates", m_contentFrame);
    m_tabBtnLayers = new QPushButton("Layers (1)", m_contentFrame);

    QString tabStyleActive = "background: #3b2212; color: #fbf5ea; border-radius: 6px; padding: 6px 10px; font-weight: 800; font-size: 11px;";
    QString tabStyleInactive = "background: #e6d3ba; color: #5c351f; border-radius: 6px; padding: 6px 10px; font-weight: 800; font-size: 11px;";

    m_tabBtnTemplates->setStyleSheet(tabStyleActive);
    m_tabBtnLayers->setStyleSheet(tabStyleInactive);

    connect(m_tabBtnTemplates, &QPushButton::clicked, [this, tabStyleActive, tabStyleInactive]() {
        m_tabStack->setCurrentIndex(0);
        m_tabBtnTemplates->setStyleSheet(tabStyleActive);
        m_tabBtnLayers->setStyleSheet(tabStyleInactive);
    });

    connect(m_tabBtnLayers, &QPushButton::clicked, [this, tabStyleActive, tabStyleInactive]() {
        m_tabStack->setCurrentIndex(1);
        m_tabBtnTemplates->setStyleSheet(tabStyleInactive);
        m_tabBtnLayers->setStyleSheet(tabStyleActive);
    });

    tabSwitcher->addWidget(m_tabBtnTemplates);
    tabSwitcher->addWidget(m_tabBtnLayers);

    QPushButton *btnClose = new QPushButton("✕", m_contentFrame);
    btnClose->setFixedSize(22, 22);
    btnClose->setStyleSheet("background: transparent; border: none; font-size: 13px; font-weight: bold; color: #7a5c43;");
    connect(btnClose, &QPushButton::clicked, this, &LayersDock::toggleOpen);

    topRow->addLayout(tabSwitcher);
    topRow->addStretch();
    topRow->addWidget(btnClose);
    contentLayout->addLayout(topRow);

    m_tabStack = new QStackedWidget(m_contentFrame);
    m_tabStack->addWidget(createTemplatesTab());
    m_tabStack->addWidget(createLayersTab());

    contentLayout->addWidget(m_tabStack);
    rootLayout->addWidget(m_contentFrame);
}

void LayersDock::toggleOpen() {
    m_isOpen = !m_isOpen;
    m_contentFrame->setVisible(m_isOpen);
    m_handleBtn->setText(m_isOpen ? "▶\n\nL\nA\nY\nE\nR\nS" : "◀\n\nL\nA\nY\nE\nR\nS");
}

QWidget *LayersDock::createTemplatesTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel *lblTarget = new QLabel("APPLY TEMPLATE TO", tab);
    lblTarget->setStyleSheet("font-size: 10px; font-weight: 800; color: #7a5c43;");
    layout->addWidget(lblTarget);

    QHBoxLayout *sideToggle = new QHBoxLayout();
    m_btnSideLeft = new QPushButton("Page 1 (Left)", tab);
    m_btnSideRight = new QPushButton("Page 2 (Right)", tab);

    QString activeSideStyle = "background: #3b2212; color: #fbf5ea; border-radius: 6px; padding: 6px; font-weight: 800; font-size: 11px;";
    QString inactiveSideStyle = "background: #f3e9d8; color: #3b2212; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 6px; font-weight: 800; font-size: 11px;";

    m_btnSideLeft->setStyleSheet(activeSideStyle);
    m_btnSideRight->setStyleSheet(inactiveSideStyle);

    connect(m_btnSideLeft, &QPushButton::clicked, [this, activeSideStyle, inactiveSideStyle]() {
        m_activeSide = "left";
        m_btnSideLeft->setStyleSheet(activeSideStyle);
        m_btnSideRight->setStyleSheet(inactiveSideStyle);
        emit activeSideChanged("left");
    });

    connect(m_btnSideRight, &QPushButton::clicked, [this, activeSideStyle, inactiveSideStyle]() {
        m_activeSide = "right";
        m_btnSideRight->setStyleSheet(activeSideStyle);
        m_btnSideLeft->setStyleSheet(inactiveSideStyle);
        emit activeSideChanged("right");
    });

    sideToggle->addWidget(m_btnSideLeft);
    sideToggle->addWidget(m_btnSideRight);
    layout->addLayout(sideToggle);

    QLabel *lblPresets = new QLabel("STORYBOOK PAGE PRESETS", tab);
    lblPresets->setStyleSheet("font-size: 10px; font-weight: 800; color: #7a5c43;");
    layout->addWidget(lblPresets);

    struct TemplateEntry { QString title; QString desc; QString id; };
    QList<TemplateEntry> presets = {
        {"Top Scene / Bottom Zone", "Upper landscape illustration with reading/writing card below", "top_art_bottom_text"},
        {"Top Zone / Bottom Scene", "Upper story block with lower ground/landscape illustration", "bottom_art_top_text"},
        {"Classic Framed Page", "Elegant framed page with inset dashed contour", "classic_arch_story"},
        {"Storyboard (2 Panels)", "Two stacked picture frames for sequential scene drawing", "storyboard_panels"},
        {"Character Spot & Card", "Centered circular portrait vignette over a wide card", "spot_rhyme"},
        {"Vertical Columns (50/50)", "Two side-by-side vertical columns centered on page", "split_vertical"},
        {"Spotless Open Drawing", "Clear all template frames on this page for freehand art", "spotless_canvas"}
    };

    for (const auto &p : presets) {
        QPushButton *btn = new QPushButton(tab);
        btn->setStyleSheet(
            "QPushButton { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 8px; padding: 8px; text-align: left; }"
            "QPushButton:hover { background: #ffffff; border-color: #8b5b32; }"
        );

        QVBoxLayout *btnLayout = new QVBoxLayout(btn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(2);

        QLabel *t = new QLabel(p.title, btn);
        t->setStyleSheet("font-weight: 800; font-size: 11px; color: #3b2212;");
        QLabel *d = new QLabel(p.desc, btn);
        d->setStyleSheet("font-size: 9px; color: #7a5c43;");
        d->setWordWrap(true);

        btnLayout->addWidget(t);
        btnLayout->addWidget(d);

        connect(btn, &QPushButton::clicked, [this, p]() {
            emit applyTemplateRequested(m_activeSide, p.id);
        });

        layout->addWidget(btn);
    }

    layout->addStretch();
    return tab;
}

QWidget *LayersDock::createLayersTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    QPushButton *btnAdd = new QPushButton("+ New Layer", tab);
    btnAdd->setStyleSheet("background: #f3dfba; border: 1.5px solid #8c633f; border-radius: 6px; padding: 7px; font-weight: 800; color: #3b2212;");
    connect(btnAdd, &QPushButton::clicked, this, &LayersDock::addLayerRequested);
    layout->addWidget(btnAdd);

    m_layersListWidget = new QListWidget(tab);
    layout->addWidget(m_layersListWidget);

    QHBoxLayout *reorderLayout = new QHBoxLayout();
    QPushButton *btnUp = new QPushButton("▲ Move Up", tab);
    QPushButton *btnDown = new QPushButton("▼ Move Down", tab);
    btnUp->setStyleSheet("background: #f3e9d8; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 5px; font-weight: 800;");
    btnDown->setStyleSheet("background: #f3e9d8; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 5px; font-weight: 800;");

    connect(btnUp, &QPushButton::clicked, [this]() {
        int row = m_layersListWidget->currentRow();
        if (row > 0) {
            emit reorderLayersRequested(row, row - 1);
        }
    });

    connect(btnDown, &QPushButton::clicked, [this]() {
        int row = m_layersListWidget->currentRow();
        if (row >= 0 && row < m_layers.size() - 1) {
            emit reorderLayersRequested(row, row + 1);
        }
    });

    reorderLayout->addWidget(btnUp);
    reorderLayout->addWidget(btnDown);
    layout->addLayout(reorderLayout);

    return tab;
}

void LayersDock::setLayers(const QList<CanvasLayer> &layers, const QString &activeLayerId) {
    m_layers = layers;
    m_activeLayerId = activeLayerId;
    m_tabBtnLayers->setText(QString("Layers (%1)").arg(layers.size()));
    refreshLayersList();
}

void LayersDock::setPageNumbers(int leftNum, int rightNum) {
    m_btnSideLeft->setText(QString("Page %1 (Left)").arg(leftNum));
    m_btnSideRight->setText(QString("Page %2 (Right)").arg(rightNum));
}

QString LayersDock::activeSide() const {
    return m_activeSide;
}

void LayersDock::refreshLayersList() {
    m_layersListWidget->clear();

    for (int i = 0; i < m_layers.size(); ++i) {
        const auto &layer = m_layers[i];

        QListWidgetItem *item = new QListWidgetItem(m_layersListWidget);
        QWidget *rowWidget = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(4, 2, 4, 2);
        rowLayout->setSpacing(6);

        QLabel *lbl = new QLabel(layer.name, rowWidget);
        lbl->setStyleSheet(layer.id == m_activeLayerId ? "font-weight: 800; color: #8b5b32;" : "font-weight: bold; color: #3b2212;");

        QPushButton *btnVis = new QPushButton(layer.visible ? "👁" : "Ø", rowWidget);
        btnVis->setFixedSize(24, 24);
        btnVis->setStyleSheet(layer.visible ? "background: transparent; border: none; font-size: 12px;" : "background: transparent; border: none; font-size: 12px; color: #dc2626;");
        connect(btnVis, &QPushButton::clicked, [this, layer]() {
            emit toggleVisibilityRequested(layer.id);
        });

        QPushButton *btnLock = new QPushButton(layer.locked ? "🔒" : "🔓", rowWidget);
        btnLock->setFixedSize(24, 24);
        btnLock->setStyleSheet("background: transparent; border: none; font-size: 12px;");
        connect(btnLock, &QPushButton::clicked, [this, layer]() {
            emit toggleLockRequested(layer.id);
        });

        QPushButton *btnRen = new QPushButton("✎", rowWidget);
        btnRen->setFixedSize(24, 24);
        btnRen->setStyleSheet("background: transparent; border: none; font-size: 12px;");
        connect(btnRen, &QPushButton::clicked, [this, layer]() {
            bool ok;
            QString text = QInputDialog::getText(this, "Rename Layer", "New layer name:", QLineEdit::Normal, layer.name, &ok);
            if (ok && !text.trimmed().isEmpty()) {
                emit renameLayerRequested(layer.id, text.trimmed());
            }
        });

        QPushButton *btnDel = new QPushButton("✕", rowWidget);
        btnDel->setFixedSize(24, 24);
        btnDel->setStyleSheet("background: transparent; border: none; font-size: 12px; color: #b91c1c; font-weight: 800;");
        btnDel->setEnabled(m_layers.size() > 1);
        connect(btnDel, &QPushButton::clicked, [this, layer]() {
            emit deleteLayerRequested(layer.id);
        });

        rowLayout->addWidget(lbl, 1);
        rowLayout->addWidget(btnVis);
        rowLayout->addWidget(btnLock);
        rowLayout->addWidget(btnRen);
        rowLayout->addWidget(btnDel);

        item->setSizeHint(rowWidget->sizeHint());
        m_layersListWidget->addItem(item);
        m_layersListWidget->setItemWidget(item, rowWidget);

        if (layer.id == m_activeLayerId) {
            m_layersListWidget->setCurrentItem(item);
        }
    }

    connect(m_layersListWidget, &QListWidget::currentRowChanged, [this](int row) {
        if (row >= 0 && row < m_layers.size()) {
            emit selectLayerRequested(m_layers[row].id);
        }
    });
}