#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include "types.h"

// Writing suggestions from Claude, run through the Claude Code CLI installed on this
// computer (`claude -p`), so it uses her Claude Code sign-in rather than an API key.
// Nothing is sent until one of the helper buttons is pressed.
class StoryHelper : public QObject {
    Q_OBJECT

public:
    enum class Task {
        Whimsical,
        Rhyme,
        SillyWords,
        WhatNext,
        Simpler,
        TitleIdeas,
        Custom
    };

    struct Request {
        Task task = Task::Custom;
        QString passage;
        QString customRequest;
        QString bookTitle;
        QString storySoFar;
        QString readerAge = "5-8";
    };

    explicit StoryHelper(QObject *parent = nullptr);
    ~StoryHelper() override;

    static bool taskNeedsPassage(Task task);
    static QString resolveClaudePath(const QString &configuredPath);
    // Runs `claude --version`; returns false with a readable reason if it can't.
    static bool checkInstall(const QString &claudePath, QString *versionOrError);

    static QString systemPrompt(const QString &readerAge);
    static QString buildPrompt(const Request &request);
    static bool parseResult(const QByteArray &output, QString *text, QString *error);
    // The book's words in reading order, for context.
    static QString storyText(const BookProject &project);

    void start(const Request &request, const QString &claudePath, const QString &model);
    void cancel();
    bool isBusy() const;

signals:
    void finished(const QString &suggestion);
    void failed(const QString &message);

private:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    QProcess *m_process = nullptr;
    QTimer m_timeout;
    bool m_cancelled = false;
};
