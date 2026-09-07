#include "new_book_dialog.h"
#include <QDateTime>

NewBookDialog::NewBookDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
}

void NewBookDialog::setupUi() {
    setWindowTitle("New Story Notebook");
    setFixedSize(620, 560);
    setStyleSheet(
        "QDialog { background: #fbf5ea; color: #3b2212; }"
        "QLabel { color: #3b2212; font-weight: bold; font-size: 11px; }"
        "QLineEdit { background: #ffffff; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 8px 10px; color: #3b2212; font-size: 12px; font-weight: 600; }"
        "QLineEdit:focus { border-color: #8b5b32; }"
        "QPushButton { background: #fbf5ea; border: 1.5px solid #8d6b4f; border-radius: 6px; padding: 7px 16px; font-weight: 800; color: #3b2212; font-size: 12px; }"
        "QPushButton:hover { background: #ffffff; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    QLabel *lblHeading = new QLabel("Create New Storybook", this);
    lblHeading->setStyleSheet("font-size: 18px; font-weight: 900; color: #3b2212;");
    QLabel *lblSub = new QLabel("Configure your title, dedication author, and book aesthetic theme.", this);
    lblSub->setStyleSheet("font-size: 11px; color: #7a5c43; font-weight: normal;");

    mainLayout->addWidget(lblHeading);
    mainLayout->addWidget(lblSub);

    QHBoxLayout *fieldsLayout = new QHBoxLayout();
    fieldsLayout->setSpacing(12);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);
    titleLayout->addWidget(new QLabel("NOTEBOOK TITLE *", this));
    m_editTitle = new QLineEdit(this);
    m_editTitle->setPlaceholderText("e.g. The Fox & The Firefly");
    titleLayout->addWidget(m_editTitle);
    fieldsLayout->addLayout(titleLayout);

    QVBoxLayout *authorLayout = new QVBoxLayout();
    authorLayout->setSpacing(4);
    authorLayout->addWidget(new QLabel("AUTHOR / DEDICATION", this));
    m_editAuthor = new QLineEdit(this);
    m_editAuthor->setPlaceholderText("e.g. For our son");
    authorLayout->addWidget(m_editAuthor);
    fieldsLayout->addLayout(authorLayout);

    mainLayout->addLayout(fieldsLayout);

    QLabel *lblTheme = new QLabel("CHOOSE BOOK THEME & CORNERS", this);
    mainLayout->addWidget(lblTheme);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget *themeContainer = new QWidget(scroll);
    QGridLayout *grid = new QGridLayout(themeContainer);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(8);

    m_themeGroup = new QButtonGroup(this);
    m_themeGroup->setExclusive(true);

    int r = 0, c = 0;
    for (int i = 0; i < THEMES.size(); ++i) {
        const auto &theme = THEMES[i];

        QPushButton *btn = new QPushButton(themeContainer);
        btn->setCheckable(true);
        btn->setFixedHeight(74);

        QString qss = QString(
            "QPushButton { background: #fdfaf5; border: 2px solid #dcc6ab; border-radius: 8px; text-align: left; padding: 6px; } "
            "QPushButton:hover { border-color: #8b5b32; background: #ffffff; } "
            "QPushButton:checked { border-color: #8b5b32; background: #faede0; }"
        );
        btn->setStyleSheet(qss);

        QHBoxLayout *itemLayout = new QHBoxLayout(btn);
        itemLayout->setContentsMargins(6, 4, 6, 4);
        itemLayout->setSpacing(10);

        QFrame *spinePreview = new QFrame(btn);
        spinePreview->setFixedSize(36, 50);
        spinePreview->setStyleSheet(QString("background: %1; border-radius: 4px; border: 1px solid #1c0e07;").arg(theme.spineColor.name()));

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(2);

        QLabel *tName = new QLabel(theme.name, btn);
        tName->setStyleSheet("font-size: 11px; font-weight: 800; color: #3b2212; background: transparent;");
        QLabel *tTag = new QLabel(theme.tagline, btn);
        tTag->setStyleSheet("font-size: 9px; color: #7a5c43; background: transparent;");
        tTag->setWordWrap(true);

        textLayout->addWidget(tName);
        textLayout->addWidget(tTag);

        itemLayout->addWidget(spinePreview);
        itemLayout->addLayout(textLayout, 1);

        m_themeGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, [this, theme]() {
            m_selectedThemeId = theme.id;
        });

        if (theme.id == m_selectedThemeId) {
            btn->setChecked(true);
        }

        grid->addWidget(btn, r, c);
        if (++c > 1) { c = 0; ++r; }
    }

    scroll->setWidget(themeContainer);
    mainLayout->addWidget(scroll, 1);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->addStretch();

    QPushButton *btnCancel = new QPushButton("Cancel", this);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *btnSubmit = new QPushButton("Open Notebook", this);
    btnSubmit->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f3dfba, stop:1 #dfbe87); border: 1.5px solid #8c633f; border-radius: 6px; padding: 7px 20px; font-weight: 900; color: #3b2212;");
    connect(btnSubmit, &QPushButton::clicked, [this]() {
        if (!m_editTitle->text().trimmed().isEmpty()) {
            accept();
        }
    });

    actionsLayout->addWidget(btnCancel);
    actionsLayout->addWidget(btnSubmit);
    mainLayout->addLayout(actionsLayout);
}

BookProject NewBookDialog::createProject() const {
    BookProject proj;
    proj.id = QString("book-%1").arg(QDateTime::currentMSecsSinceEpoch());
    proj.title = m_editTitle->text().trimmed();
    proj.author = m_editAuthor->text().trimmed().isEmpty() ? "Author" : m_editAuthor->text().trimmed();
    proj.themeId = m_selectedThemeId;
    proj.createdAt = QDate::currentDate().toString("MM/dd/yyyy");

    BookSpread s1;
    s1.id = "s-1";
    s1.leftPageNum = 1;
    s1.rightPageNum = 2;
    s1.activeLayerId = "layer-1";
    CanvasLayer l1;
    l1.id = "layer-1";
    l1.name = "Artwork Layer";
    s1.layers.append(l1);

    BookSpread s2;
    s2.id = "s-2";
    s2.leftPageNum = 3;
    s2.rightPageNum = 4;
    s2.activeLayerId = "layer-1";
    s2.layers.append(l1);

    proj.spreads.append(s1);
    proj.spreads.append(s2);

    return proj;
}