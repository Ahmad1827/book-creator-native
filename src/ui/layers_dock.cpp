#include "layers_dock.h"
#include "sketch.h"
#include <QButtonGroup>
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
    m_handleBtn->setFixedWidth(38);
    m_handleBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_handleBtn->setText("▶\n\nP\na\ng\ne\n\nl\na\ny\no\nu\nt\ns");
    m_handleBtn->setProperty("role", "ghost");
    m_handleBtn->setFont(Sketch::headingFont(17));
    m_handleBtn->setToolTip("Toggle Layout & Layers Panel");
    connect(m_handleBtn, &QPushButton::clicked, this, &LayersDock::toggleOpen);
    rootLayout->addWidget(m_handleBtn);

    m_contentFrame = new SketchPanel(this, Sketch::kCard, SketchPanel::LeftEdge);
    m_contentFrame->setFixedWidth(340);

    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentFrame);
    contentLayout->setContentsMargins(14, 14, 14, 14);
    contentLayout->setSpacing(12);

    QHBoxLayout *topRow = new QHBoxLayout();
    QHBoxLayout *tabSwitcher = new QHBoxLayout();
    tabSwitcher->setSpacing(6);

    m_tabBtnTemplates = new QPushButton("Page Layouts", m_contentFrame);
    m_tabBtnLayers = new QPushButton("Layers (1)", m_contentFrame);
    QButtonGroup *tabs = new QButtonGroup(m_contentFrame);
    for (QPushButton *tab : {m_tabBtnTemplates, m_tabBtnLayers}) {
        tab->setProperty("role", "chip");
        tab->setCheckable(true);
        tabs->addButton(tab);
    }
    m_tabBtnTemplates->setChecked(true);

    connect(m_tabBtnTemplates, &QPushButton::clicked, [this]() {
        m_tabStack->setCurrentIndex(0);
    });

    connect(m_tabBtnLayers, &QPushButton::clicked, [this]() {
        m_tabStack->setCurrentIndex(1);
    });

    tabSwitcher->addWidget(m_tabBtnTemplates);
    tabSwitcher->addWidget(m_tabBtnLayers);

    QPushButton *btnClose = new QPushButton("✕", m_contentFrame);
    btnClose->setFixedSize(34, 34);
    btnClose->setProperty("role", "ghost");
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
    m_handleBtn->setText(m_isOpen ? "▶\n\nP\na\ng\ne\n\nl\na\ny\no\nu\nt\ns" : "◀\n\nP\na\ng\ne\n\nl\na\ny\no\nu\nt\ns");

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

    QLabel *lblTarget = Sketch::section("Which page?", tab);
    layout->addWidget(lblTarget);

    QHBoxLayout *sideToggle = new QHBoxLayout();
    sideToggle->setSpacing(8);
    m_btnSideLeft = new QPushButton("Page 1 (Left)", tab);
    m_btnSideRight = new QPushButton("Page 2 (Right)", tab);

    QButtonGroup *sides = new QButtonGroup(tab);
    for (QPushButton *side : {m_btnSideLeft, m_btnSideRight}) {
        side->setProperty("role", "chip");
        side->setCheckable(true);
        side->setFont(Sketch::headingFont(16));
        sides->addButton(side);
    }
    m_btnSideLeft->setChecked(true);

    connect(m_btnSideLeft, &QPushButton::clicked, [this]() {
        m_activeSide = "left";
        emit activeSideChanged("left");
    });

    connect(m_btnSideRight, &QPushButton::clicked, [this]() {
        m_activeSide = "right";
        emit activeSideChanged("right");
    });

    sideToggle->addWidget(m_btnSideLeft);
    sideToggle->addWidget(m_btnSideRight);
    layout->addLayout(sideToggle);

    QLabel *lblPresets = Sketch::section("Layouts", tab);
    layout->addWidget(lblPresets);

    struct TemplateEntry { QString title; QString desc; QString id; };
    QList<TemplateEntry> presets = {
        {"Big Picture & Caption", "Large illustration with a line of story text underneath", "big_art_caption"},
        {"Full-Page Picture", "One illustration filling the whole page", "full_page_art"},
        {"Story Text Page", "A page just for words, facing a picture", "text_page"},
        {"Picture Across Both Pages", "One big scene spanning the whole spread", "spread_art"},
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
        btn->setProperty("role", "card");

        QVBoxLayout *inner = new QVBoxLayout(btn);
        inner->setContentsMargins(16, 10, 16, 14);
        inner->setSpacing(4);

        QLabel *t = new QLabel(p.title, btn);
        t->setAttribute(Qt::WA_TransparentForMouseEvents);
        t->setFont(Sketch::headingFont(18));
        
        QLabel *d = new QLabel(p.desc, btn);
        d->setAttribute(Qt::WA_TransparentForMouseEvents);
        Sketch::styleHint(d);
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
    btnAdd->setProperty("role", "primary");
    connect(btnAdd, &QPushButton::clicked, this, &LayersDock::addLayerRequested);
    layout->addWidget(btnAdd);

    m_layersListWidget = new QListWidget(tab);
    layout->addWidget(m_layersListWidget, 1);

    QHBoxLayout *reorderLayout = new QHBoxLayout();
    reorderLayout->setSpacing(8);
    QPushButton *btnUp = new QPushButton("▲ Move Up", tab);
    QPushButton *btnDown = new QPushButton("▼ Move Down", tab);


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
    m_btnSideLeft->setText(leftNum > 0 ? QString("Page %1 (Left)").arg(leftNum) : QString("Back Cover"));
    m_btnSideRight->setText(rightNum > 0 ? QString("Page %1 (Right)").arg(rightNum) : QString("Front Cover"));
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
        lbl->setFont(Sketch::headingFont(layer.id == m_activeLayerId ? 19 : 17));

        QPushButton *btnVis = new QPushButton(layer.visible ? "👁" : "Ø", rowWidget);
        btnVis->setFixedSize(30, 30);
        btnVis->setProperty("role", "ghost");
        connect(btnVis, &QPushButton::clicked, [this, layer]() {
            emit toggleVisibilityRequested(layer.id);
        });

        QPushButton *btnLock = new QPushButton(layer.locked ? "🔒" : "🔓", rowWidget);
        btnLock->setFixedSize(30, 30);
        btnLock->setProperty("role", "ghost");
        connect(btnLock, &QPushButton::clicked, [this, layer]() {
            emit toggleLockRequested(layer.id);
        });

        QPushButton *btnRen = new QPushButton("✎", rowWidget);
        btnRen->setFixedSize(30, 30);
        btnRen->setProperty("role", "ghost");
        connect(btnRen, &QPushButton::clicked, [this, layer]() {
            bool ok;
            QString text = QInputDialog::getText(this, "Rename Layer", "New layer name:", QLineEdit::Normal, layer.name, &ok);
            if (ok && !text.trimmed().isEmpty()) {
                emit renameLayerRequested(layer.id, text.trimmed());
            }
        });

        QPushButton *btnDel = new QPushButton("✕", rowWidget);
        btnDel->setFixedSize(30, 30);
        btnDel->setProperty("role", "ghost");
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