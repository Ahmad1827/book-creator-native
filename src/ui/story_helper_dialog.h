#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <functional>
#include "story_helper.h"

class BookCanvasView;

// A floating window that stays open beside the book while she writes.
class StoryHelperDialog : public QDialog {
    Q_OBJECT

public:
    // storyContext returns {book title, the story so far}.
    StoryHelperDialog(BookCanvasView *canvas,
                      std::function<QPair<QString, QString>()> storyContext,
                      QWidget *parent = nullptr);

    void pullSelectedText();

signals:
    void settingsRequested();

private:
    void ask(StoryHelper::Task task);
    void setBusy(bool busy, const QString &status = QString());

    BookCanvasView *m_canvas;
    std::function<QPair<QString, QString>()> m_storyContext;
    StoryHelper *m_helper;

    QPlainTextEdit *m_editPassage;
    QLineEdit *m_editCustom;
    QPlainTextEdit *m_editSuggestion;
    QLabel *m_lblStatus;
    QProgressBar *m_progress;
    QPushButton *m_btnCancel;
    QList<QPushButton*> m_actionButtons;
    QPushButton *m_btnReplace;
    QPushButton *m_btnInsert;
    QPushButton *m_btnCopy;
};
