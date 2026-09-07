#include "layers_dock.h"
#include <QInputDialog>
#include <QScrollArea>

LayersDock::LayersDock(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

void LayersDock::setupUi() {
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_handleBtn = new QPushButton(this);
    m_handleBtn->setFixedWidth(30);
    m_handleBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_handleBtn->setText("▶\n\nL\nA\nY\nE\nR\nS");
    m_handleBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2b170c, stop:1 #381f13); "
        "border: none; border-left: 2px solid #1c0e07; color: #dfbe87; font-weight: 800; font-size: 10px; line-height: 14px; } "
        "QPushButton:hover { color: #ffffff; background: #442617; }"
    );
    m_handleBtn->setToolTip("Toggle Layout & Layers Panel");
    connect(m_handleBtn, &QPushButton::clicked, this, &LayersDock::toggleOpen);
    rootLayout->addWidget(m_handleBtn);

    m_contentFrame = new QFrame(this);
    m_contentFrame->setFixedWidth(340);
    m_contentFrame->setStyleSheet(
        "QFrame { background: #fbf5ea; border-left: 2px solid #8d6b4f; color: #3b2212; }"
        "QListWidget { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 8px; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #ebd9c3; padding: 4px; }"
        "QListWidget::item:selected { background: #ecd9bf; }"
    );

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(14, 14, 14, 14);
    contentLayout->setSpacing(12);

    QHBoxLayout *topRow = new QHBoxLayout();
    QHBoxLayout *tabSwitcher = new QHBoxLayout();
    tabSwitcher->setSpacing(6);

    m_tabBtnTemplates = new QPushButton("Templates", m_contentFrame);
    m_tabBtnLayers = new QPushButton("Layers (1)", m_contentFrame);

    QString tabStyleActive = "background: #3b2212; color: #fbf5ea; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11px;";
    QString tabStyleInactive = "background: #e6d3ba; color: #5c351f; border-radius: 6px; padding: 6px 14px; font-weight: 800; font-size: 11px;";

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
    btnClose->setFixedSize(24, 24);
    btnClose->setStyleSheet("background: transparent; border: none; font-size: 13px; font-weight: bold; color: #7a5c43;");
    connect(btnClose, &QPushButton::clicked, this, &LayersDock::toggleOpen);

    topRow->addLayout(tabSwitcher);
    topRow->addStretch();
    topRow->addWidget(btnClose);
    contentLayout->addLayout(topRow);

    m_tabStack = new QStackedWidget(m_contentFrame);
    m_tabStack->addWidget(createTemplatesTab());
    m_tabStack->addWidget(createLayersTab());

    contentLayout->addWidget(m_tabStack, 1);
    rootLayout->addWidget(m_contentFrame);
}

void LayersDock::animatePanel(bool open) {
    m_isOpen = open;
    m_handleBtn->setText(m_isOpen ? "▶\n\nL\nA\nY\nE\nR\nS" : "◀\n\nL\nA\nY\nE\nR\nS");

    QPropertyAnimation *anim = new QPropertyAnimation(m_contentFrame, "minimumWidth", this);
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(m_contentFrame->width());
    anim->setEndValue(open ? 340 : 0);

    connect(anim, &QPropertyAnimation::valueChanged, [this](const QVariant &val) {
        m_contentFrame->setMaximumWidth(val.toInt());
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void LayersDock::toggleOpen() {
    animatePanel(!m_isOpen);
}

QWidget *LayersDock::createTemplatesTab() {
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *tab = new QWidget(scroll);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(6, 6, 12, 6);
    layout->setSpacing(10);

    QLabel *lblTarget = new QLabel("TARGET PAGE", tab);
    lblTarget->setStyleSheet("font-size: 10px; font-weight: 800; color: #8c633f; letter-spacing: 0.5px; padding-left: 4px;");
    layout->addWidget(lblTarget);

    QHBoxLayout *sideToggle = new QHBoxLayout();
    sideToggle->setSpacing(8);
    m_btnSideLeft = new QPushButton("Page 1 (Left)", tab);
    m_btnSideRight = new QPushButton("Page 2 (Right)", tab);

    QString activeSideStyle = "background: #3b2212; color: #fbf5ea; border-radius: 6px; padding: 8px 0; font-weight: 800; font-size: 11px;";
    QString inactiveSideStyle = "background: #f3e9d8; color: #3b2212; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 8px 0; font-weight: 800; font-size: 11px;";

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

    QLabel *lblPresets = new QLabel("PAGE PRESETS", tab);
    lblPresets->setStyleSheet("font-size: 10px; font-weight: 800; color: #8c633f; letter-spacing: 0.5px; padding-left: 4px; margin-top: 6px;");
    layout->addWidget(lblPresets);

    struct TemplateEntry { QString title; QString desc; QString id; };
    QList<TemplateEntry> presets = {
        {"Top Scene / Bottom Text", "Illustration on top, text card below", "top_art_bottom_text"},
        {"Top Text / Bottom Scene", "Text card on top, illustration below", "bottom_art_top_text"},
        {"Framed Story Page", "Inner border frame with dashed margin", "classic_arch_story"},
        {"2 Panels (Vertical)", "Dual stacked comic storyboard frames", "storyboard_panels"},
        {"Spotlight & Note", "Round portrait vignette above a plaque", "spot_rhyme"},
        {"50 / 50 Columns", "Two side-by-side vertical columns", "split_vertical"},
        {"Clear Layout", "Clear preset frames for freehand drawing", "spotless_canvas"}
    };

    for (const auto &p : presets) {
        QPushButton *btn = new QPushButton(tab);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setStyleSheet(
            "QPushButton { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 8px; } "
            "QPushButton:hover { background: #ffffff; border-color: #8b5b32; } "
            "QPushButton:pressed { background: #faede0; }"
        );

        QVBoxLayout *inner = new QVBoxLayout(btn);
        inner->setContentsMargins(14, 10, 14, 10);
        inner->setSpacing(4);

        QLabel *t = new QLabel(p.title, btn);
        t->setAttribute(Qt::WA_TransparentForMouseEvents);
        t->setStyleSheet("font-weight: 800; font-size: 11px; color: #3b2212; background: transparent; border: none;");
        
        QLabel *d = new QLabel(p.desc, btn);
        d->setAttribute(Qt::WA_TransparentForMouseEvents);
        d->setStyleSheet("font-size: 10px; color: #7a5c43; background: transparent; border: none;");
        d->setWordWrap(true);

        inner->addWidget(t);
        inner->addWidget(d);

        connect(btn, &QPushButton::clicked, [this, p]() {
            emit applyTemplateRequested(m_activeSide, p.id);
        });

        layout->addWidget(btn);
    }

    layout->addStretch();
    scroll->setWidget(tab);
    return scroll;
}

QWidget *LayersDock::createLayersTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(10);

    QPushButton *btnAdd = new QPushButton("+ New Layer", tab);
    btnAdd->setStyleSheet(
        "QPushButton { background: #f3dfba; border: 1.5px solid #8c633f; border-radius: 6px; padding: 8px 12px; font-weight: 800; color: #3b2212; } "
        "QPushButton:hover { background: #faede0; border-color: #5c351f; }"
    );
    connect(btnAdd, &QPushButton::clicked, this, &LayersDock::addLayerRequested);
    layout->addWidget(btnAdd);

    m_layersListWidget = new QListWidget(tab);
    layout->addWidget(m_layersListWidget, 1);

    QHBoxLayout *reorderLayout = new QHBoxLayout();
    reorderLayout->setSpacing(8);
    QPushButton *btnUp = new QPushButton("▲ Move Up", tab);
    QPushButton *btnDown = new QPushButton("▼ Move Down", tab);
    btnUp->setStyleSheet("background: #f3e9d8; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 6px 0; font-weight: 800; font-size: 11px;");
    btnDown->setStyleSheet("background: #f3e9d8; border: 1.5px solid #d4be9f; border-radius: 6px; padding: 6px 0; font-weight: 800; font-size: 11px;");

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
        rowLayout->setContentsMargins(6, 4, 6, 4);
        rowLayout->setSpacing(6);

        QLabel *lbl = new QLabel(layer.name, rowWidget);
        lbl->setStyleSheet(layer.id == m_activeLayerId ? "font-weight: 800; color: #8b5b32;" : "font-weight: bold; color: #3b2212;");

        QPushButton *btnVis = new QPushButton(layer.visible ? "👁" : "Ø", rowWidget);
        btnVis->setFixedSize(24, 24);
        btnVis->setStyleSheet(layer.visible ? "background: transparent; border: none; font-size: 11px;" : "background: transparent; border: none; font-size: 11px; color: #dc2626;");
        connect(btnVis, &QPushButton::clicked, [this, layer]() {
            emit toggleVisibilityRequested(layer.id);
        });

        QPushButton *btnLock = new QPushButton(layer.locked ? "🔒" : "🔓", rowWidget);
        btnLock->setFixedSize(24, 24);
        btnLock->setStyleSheet("background: transparent; border: none; font-size: 11px;");
        connect(btnLock, &QPushButton::clicked, [this, layer]() {
            emit toggleLockRequested(layer.id);
        });

        QPushButton *btnRen = new QPushButton("✎", rowWidget);
        btnRen->setFixedSize(24, 24);
        btnRen->setStyleSheet("background: transparent; border: none; font-size: 11px;");
        connect(btnRen, &QPushButton::clicked, [this, layer]() {
            bool ok;
            QString text = QInputDialog::getText(this, "Rename Layer", "New layer name:", QLineEdit::Normal, layer.name, &ok);
            if (ok && !text.trimmed().isEmpty()) {
                emit renameLayerRequested(layer.id, text.trimmed());
            }
        });

        QPushButton *btnDel = new QPushButton("✕", rowWidget);
        btnDel->setFixedSize(24, 24);
        btnDel->setStyleSheet("background: transparent; border: none; font-size: 11px; color: #b91c1c; font-weight: 800;");
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