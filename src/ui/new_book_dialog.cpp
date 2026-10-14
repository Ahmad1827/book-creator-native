#include "new_book_dialog.h"
#include "sketch.h"
#include <QDateTime>

NewBookDialog::NewBookDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
}

void NewBookDialog::setupUi() {
    setWindowTitle("A Brand New Book");
    setFixedSize(720, 640);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    QLabel *lblHeading = new QLabel("A Brand New Book!", this);
    lblHeading->setFont(Sketch::titleFont(34));
    QLabel *lblSub = new QLabel("Give it a name, choose its size and pick a look. You can change the look later.", this);
    Sketch::styleHint(lblSub);

    mainLayout->addWidget(lblHeading);
    mainLayout->addWidget(lblSub);

    QHBoxLayout *fieldsLayout = new QHBoxLayout();
    fieldsLayout->setSpacing(12);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);
    titleLayout->addWidget(Sketch::section("Title", this));
    m_editTitle = new QLineEdit(this);
    m_editTitle->setPlaceholderText("e.g. The Fox & The Firefly");
    titleLayout->addWidget(m_editTitle);
    fieldsLayout->addLayout(titleLayout);

    QVBoxLayout *authorLayout = new QVBoxLayout();
    authorLayout->setSpacing(4);
    authorLayout->addWidget(Sketch::section("Written by", this));
    m_editAuthor = new QLineEdit(this);
    m_editAuthor->setPlaceholderText("e.g. Mum");
    authorLayout->addWidget(m_editAuthor);
    fieldsLayout->addLayout(authorLayout);

    QVBoxLayout *formatLayout = new QVBoxLayout();
    formatLayout->setSpacing(4);
    formatLayout->addWidget(Sketch::section("Book size", this));
    m_comboFormat = new QComboBox(this);
    for (const auto &format : BOOK_FORMATS) m_comboFormat->addItem(format.name, format.id);
    m_comboFormat->setToolTip("The printed page size. Square 8.5 in is the most common picture-book size.");
    formatLayout->addWidget(m_comboFormat);
    fieldsLayout->addLayout(formatLayout);

    mainLayout->addLayout(fieldsLayout);

    QLabel *lblTheme = Sketch::section("Pick a look", this);
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
        btn->setFixedHeight(86);

        btn->setProperty("role", "card");

        QHBoxLayout *itemLayout = new QHBoxLayout(btn);
        itemLayout->setContentsMargins(6, 4, 6, 4);
        itemLayout->setSpacing(10);

        QLabel *spinePreview = new QLabel(btn);
        spinePreview->setFixedSize(46, 62);
        spinePreview->setPixmap(Sketch::spinePicture(theme.spineColor, theme.paperBg, theme.accentColor, QSize(46, 62)));
        spinePreview->setAttribute(Qt::WA_TransparentForMouseEvents);

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(2);

        QLabel *tName = new QLabel(theme.name, btn);
        tName->setFont(Sketch::headingFont(19));
        QLabel *tTag = new QLabel(theme.tagline, btn);
        Sketch::styleHint(tTag);
        tTag->setFont(Sketch::bodyFont(13));
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

    QPushButton *btnSubmit = new QPushButton("Make My Book!", this);
    btnSubmit->setProperty("role", "primary");
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
    proj.formatId = m_comboFormat->currentData().toString();
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