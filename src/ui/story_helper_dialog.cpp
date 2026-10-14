#include "story_helper_dialog.h"
#include "book_canvas_view.h"
#include "app_settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QClipboard>

StoryHelperDialog::StoryHelperDialog(BookCanvasView *canvas,
                                     std::function<QPair<QString, QString>()> storyContext,
                                     QWidget *parent)
    : QDialog(parent),
      m_canvas(canvas),
      m_storyContext(std::move(storyContext)),
      m_helper(new StoryHelper(this)) {

    setWindowTitle("Story Helper");
    setModal(false);
    resize(460, 720);
    setStyleSheet(
        "QDialog { background: #fbf5ea; }"
        "QLabel { color: #3b2212; }"
        "QPlainTextEdit, QLineEdit { background: #ffffff; border: 1.5px solid #dcc6ab; border-radius: 8px; padding: 6px; color: #3b2212; font-size: 13px; }"
        "QPushButton { background: #fdfaf5; border: 1.5px solid #dcc6ab; border-radius: 6px; padding: 7px 10px; font-weight: 800; color: #3b2212; }"
        "QPushButton:hover { background: #ffffff; border-color: #8b5b32; }"
        "QPushButton:disabled { color: #b9a58f; }"
        "QProgressBar { border: none; background: #ecd9bf; border-radius: 3px; max-height: 6px; }"
        "QProgressBar::chunk { background: #8b5b32; border-radius: 3px; }"
    );

    const QString sectionStyle = "font-size: 10px; font-weight: 800; color: #8c633f; letter-spacing: 0.5px;";

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    QHBoxLayout *header = new QHBoxLayout();
    QLabel *title = new QLabel("Story Helper", this);
    title->setStyleSheet("font-size: 18px; font-weight: 900;");
    QPushButton *btnSettings = new QPushButton("Settings…", this);
    btnSettings->setStyleSheet("QPushButton { background: transparent; border: none; color: #8b5b32; text-decoration: underline; padding: 2px; }");
    connect(btnSettings, &QPushButton::clicked, this, &StoryHelperDialog::settingsRequested);
    header->addWidget(title);
    header->addStretch();
    header->addWidget(btnSettings);
    layout->addLayout(header);

    QLabel *sub = new QLabel("Ideas from Claude, through Claude Code on this computer. Take what you like and change the rest!", this);
    sub->setWordWrap(true);
    sub->setStyleSheet("color: #7a5c43; font-size: 11px;");
    layout->addWidget(sub);

    QHBoxLayout *passageHeader = new QHBoxLayout();
    QLabel *lblPassage = new QLabel("YOUR TEXT", this);
    lblPassage->setStyleSheet(sectionStyle);
    QPushButton *btnPull = new QPushButton("Use Selected Text Box", this);
    btnPull->setStyleSheet("QPushButton { padding: 3px 8px; font-size: 10px; }");
    connect(btnPull, &QPushButton::clicked, this, &StoryHelperDialog::pullSelectedText);
    passageHeader->addWidget(lblPassage);
    passageHeader->addStretch();
    passageHeader->addWidget(btnPull);
    layout->addLayout(passageHeader);

    m_editPassage = new QPlainTextEdit(this);
    m_editPassage->setPlaceholderText("Select a text box on the page, or type or paste a few lines here.");
    m_editPassage->setMaximumHeight(110);
    layout->addWidget(m_editPassage);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(6);
    struct Action { QString label; StoryHelper::Task task; };
    const QList<Action> actions = {
        {"Make it Whimsical", StoryHelper::Task::Whimsical},
        {"Make it Rhyme", StoryHelper::Task::Rhyme},
        {"Invent Silly Words", StoryHelper::Task::SillyWords},
        {"What Happens Next?", StoryHelper::Task::WhatNext},
        {"Simpler Words", StoryHelper::Task::Simpler},
        {"Title Ideas", StoryHelper::Task::TitleIdeas},
    };
    for (int i = 0; i < actions.size(); ++i) {
        const auto action = actions[i];
        QPushButton *btn = new QPushButton(action.label, this);
        connect(btn, &QPushButton::clicked, this, [this, action]() { ask(action.task); });
        grid->addWidget(btn, i / 2, i % 2);
        m_actionButtons.append(btn);
    }
    layout->addLayout(grid);

    QLabel *lblCustom = new QLabel("OR ASK ANYTHING", this);
    lblCustom->setStyleSheet(sectionStyle + " margin-top: 4px;");
    layout->addWidget(lblCustom);
    QHBoxLayout *customRow = new QHBoxLayout();
    m_editCustom = new QLineEdit(this);
    m_editCustom->setPlaceholderText("e.g. Give the fox a funny catchphrase");
    QPushButton *btnAsk = new QPushButton("Ask", this);
    connect(btnAsk, &QPushButton::clicked, this, [this]() { ask(StoryHelper::Task::Custom); });
    connect(m_editCustom, &QLineEdit::returnPressed, this, [this]() { ask(StoryHelper::Task::Custom); });
    customRow->addWidget(m_editCustom, 1);
    customRow->addWidget(btnAsk);
    m_actionButtons.append(btnAsk);
    layout->addLayout(customRow);

    QHBoxLayout *statusRow = new QHBoxLayout();
    m_lblStatus = new QLabel(this);
    m_lblStatus->setWordWrap(true);
    m_lblStatus->setStyleSheet("font-size: 11px; color: #7a5c43;");
    m_btnCancel = new QPushButton("Stop", this);
    m_btnCancel->setVisible(false);
    connect(m_btnCancel, &QPushButton::clicked, this, [this]() {
        m_helper->cancel();
        setBusy(false, "Stopped.");
    });
    statusRow->addWidget(m_lblStatus, 1);
    statusRow->addWidget(m_btnCancel);
    layout->addLayout(statusRow);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 0);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    QLabel *lblSuggestion = new QLabel("SUGGESTION (you can edit it)", this);
    lblSuggestion->setStyleSheet(sectionStyle);
    layout->addWidget(lblSuggestion);
    m_editSuggestion = new QPlainTextEdit(this);
    m_editSuggestion->setPlaceholderText("Suggestions appear here.");
    layout->addWidget(m_editSuggestion, 1);

    QHBoxLayout *useRow = new QHBoxLayout();
    m_btnReplace = new QPushButton("Replace Selected Text", this);
    m_btnInsert = new QPushButton("Add as New Text Box", this);
    m_btnCopy = new QPushButton("Copy", this);
    useRow->addWidget(m_btnReplace);
    useRow->addWidget(m_btnInsert);
    useRow->addWidget(m_btnCopy);
    layout->addLayout(useRow);

    connect(m_btnReplace, &QPushButton::clicked, this, [this]() {
        const QString text = m_editSuggestion->toPlainText().trimmed();
        if (text.isEmpty()) return;
        if (m_canvas->replaceSelectedStoryText(text)) {
            m_lblStatus->setText("Replaced the selected text box. Ctrl+Z puts it back.");
        } else {
            m_lblStatus->setText("Select a text box on the page first (with the Text or Select tool).");
        }
    });
    connect(m_btnInsert, &QPushButton::clicked, this, [this]() {
        const QString text = m_editSuggestion->toPlainText().trimmed();
        if (text.isEmpty()) return;
        m_canvas->addStoryTextBox(text);
        m_lblStatus->setText("Added to the page you last clicked. Use the Select tool to move it.");
    });
    connect(m_btnCopy, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_editSuggestion->toPlainText());
        m_lblStatus->setText("Copied.");
    });

    connect(m_helper, &StoryHelper::finished, this, [this](const QString &suggestion) {
        m_editSuggestion->setPlainText(suggestion);
        setBusy(false, "Here's an idea. Edit it as much as you like.");
    });
    connect(m_helper, &StoryHelper::failed, this, [this](const QString &message) {
        setBusy(false);
        m_lblStatus->setText(message);
        m_lblStatus->setStyleSheet("font-size: 11px; color: #b91c1c;");
    });

    setBusy(false);
    pullSelectedText();
}

void StoryHelperDialog::pullSelectedText() {
    const QString text = m_canvas->selectedStoryText();
    if (!text.isEmpty()) m_editPassage->setPlainText(text);
}

void StoryHelperDialog::ask(StoryHelper::Task task) {
    if (m_helper->isBusy()) return;

    StoryHelper::Request request;
    request.task = task;
    request.passage = m_editPassage->toPlainText();
    request.customRequest = m_editCustom->text();
    request.readerAge = AppSettings::readerAge();

    if (StoryHelper::taskNeedsPassage(task) && request.passage.trimmed().isEmpty()) {
        pullSelectedText();
        request.passage = m_editPassage->toPlainText();
        if (request.passage.trimmed().isEmpty()) {
            m_lblStatus->setText("Select a text box, or type some text above, for this one.");
            return;
        }
    }
    if (task == StoryHelper::Task::Custom && request.customRequest.trimmed().isEmpty()) {
        m_editCustom->setFocus();
        return;
    }

    const auto context = m_storyContext();
    request.bookTitle = context.first;
    request.storySoFar = context.second;

    setBusy(true, "Claude is thinking…");
    const QString claude = StoryHelper::resolveClaudePath(AppSettings::claudePath());
    m_helper->start(request, claude, AppSettings::claudeModel());
}

void StoryHelperDialog::setBusy(bool busy, const QString &status) {
    for (auto *btn : m_actionButtons) btn->setEnabled(!busy);
    m_btnReplace->setEnabled(!busy);
    m_btnInsert->setEnabled(!busy);
    m_progress->setVisible(busy);
    m_btnCancel->setVisible(busy);
    m_lblStatus->setStyleSheet("font-size: 11px; color: #7a5c43;");
    m_lblStatus->setText(status);
}
