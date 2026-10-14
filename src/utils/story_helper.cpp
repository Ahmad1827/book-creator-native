#include "story_helper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>

namespace {

constexpr int kTimeoutMs = 180000;
constexpr int kMaxContextChars = 6000;

QString helperWorkDir() {
    // An empty folder, so Claude Code doesn't pick up any project files as context.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/book-creator-helper";
    QDir().mkpath(dir);
    return dir;
}

void commandFor(const QString &claudePath, const QStringList &args, QString *program, QStringList *fullArgs) {
#ifdef Q_OS_WIN
    // npm installs a claude.cmd shim, which has to go through the command interpreter.
    if (claudePath.endsWith(".cmd", Qt::CaseInsensitive) || claudePath.endsWith(".bat", Qt::CaseInsensitive)) {
        *program = "cmd.exe";
        *fullArgs = QStringList{"/c", claudePath} + args;
        return;
    }
#endif
    *program = claudePath;
    *fullArgs = args;
}

QString stripWrapping(QString text) {
    text = text.trimmed();
    if (text.startsWith("```")) {
        const int firstNewline = text.indexOf('\n');
        text = firstNewline >= 0 ? text.mid(firstNewline + 1) : QString();
        if (text.endsWith("```")) text.chop(3);
        text = text.trimmed();
    }
    return text;
}

QString notFoundMessage() {
    return "Claude Code isn't installed on this computer, or the app can't find it.\n\n"
           "Install it from claude.com/claude-code, open a terminal and run \"claude\" once to sign in, "
           "then try again. If it's installed somewhere unusual, set its location in Settings.";
}

} // namespace

StoryHelper::StoryHelper(QObject *parent)
    : QObject(parent) {
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(kTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        if (!isBusy()) return;
        m_cancelled = true;
        m_process->kill();
        emit failed("Claude took too long to answer. Please try again.");
    });
}

StoryHelper::~StoryHelper() {
    if (isBusy()) {
        m_cancelled = true;
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

bool StoryHelper::taskNeedsPassage(Task task) {
    return task == Task::Whimsical || task == Task::Rhyme || task == Task::Simpler;
}

QString StoryHelper::resolveClaudePath(const QString &configuredPath) {
    if (!configuredPath.isEmpty()) {
        return QFileInfo::exists(configuredPath) ? configuredPath : QString();
    }

    const QString onPath = QStandardPaths::findExecutable("claude");
    if (!onPath.isEmpty()) return onPath;

    // Apps started from a desktop menu often don't see the shell's PATH.
    const QString home = QDir::homePath();
    QStringList candidates = {
        home + "/.local/bin/claude",
        home + "/.claude/local/claude",
        home + "/.npm-global/bin/claude",
        "/usr/local/bin/claude",
        "/opt/homebrew/bin/claude",
    };
#ifdef Q_OS_WIN
    const QString appData = qEnvironmentVariable("APPDATA");
    candidates = {
        home + "/.local/bin/claude.exe",
        home + "/.claude/local/claude.exe",
        appData + "/npm/claude.cmd",
    };
#endif
    for (const QString &candidate : candidates) {
        if (QFileInfo(candidate).isFile()) return candidate;
    }
    return {};
}

bool StoryHelper::checkInstall(const QString &claudePath, QString *versionOrError) {
    if (claudePath.isEmpty()) {
        *versionOrError = "Claude Code wasn't found on this computer.";
        return false;
    }
    QString program;
    QStringList args;
    commandFor(claudePath, {"--version"}, &program, &args);

    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted(5000)) {
        *versionOrError = "Couldn't start Claude Code: " + process.errorString();
        return false;
    }
    if (!process.waitForFinished(15000)) {
        process.kill();
        *versionOrError = "Claude Code didn't respond.";
        return false;
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (process.exitCode() != 0 || output.isEmpty()) {
        *versionOrError = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (versionOrError->isEmpty()) *versionOrError = "Claude Code exited with an error.";
        return false;
    }
    *versionOrError = output.section('\n', 0, 0);
    return true;
}

QString StoryHelper::systemPrompt(const QString &readerAge) {
    return QString(
        "You are a warm, playful writing companion helping a parent write a picture book for children aged %1. "
        "Write in the spirit of classic mischievous British children's storytelling: inventive made-up words, gleeful "
        "exaggeration, cheeky humour, delicious sounds that are fun to read aloud, and kindness at the heart of it. "
        "Every line you write must be original. Never quote or closely imitate passages from existing books. "
        "Keep vocabulary and sentence length right for the age. Nothing scarier than gentle spookiness, and nothing "
        "unsuitable for young children. "
        "Reply with only the text that was asked for: no introduction, no explanation, no headings, no markdown, "
        "and no quotation marks around the whole answer."
    ).arg(readerAge);
}

QString StoryHelper::buildPrompt(const Request &request) {
    QString instruction;
    switch (request.task) {
    case Task::Whimsical:
        instruction = "Rewrite this passage so it is more playful and whimsical. Keep what happens and keep it about the same length.";
        break;
    case Task::Rhyme:
        instruction = "Rewrite this passage as a short rhyming verse with a bouncy rhythm that is fun to read aloud.";
        break;
    case Task::Simpler:
        instruction = "Rewrite this passage with simpler words and shorter sentences for younger children, keeping all the fun.";
        break;
    case Task::SillyWords:
        instruction = "Invent 6 delightful made-up words that would fit this story. Put each on its own line as: word - a short meaning a child would understand.";
        break;
    case Task::WhatNext:
        instruction = "Suggest 3 different ideas for what could happen on the next page. Number them 1 to 3, one or two sentences each.";
        break;
    case Task::TitleIdeas:
        instruction = "Suggest 5 title ideas for this book, one per line.";
        break;
    case Task::Custom:
        instruction = request.customRequest.trimmed();
        break;
    }

    QString prompt = instruction + "\n";
    if (!request.passage.trimmed().isEmpty()) {
        prompt += "\nPassage:\n" + request.passage.trimmed() + "\n";
    }
    if (!request.bookTitle.trimmed().isEmpty()) {
        prompt += "\nBook title: " + request.bookTitle.trimmed() + "\n";
    }
    QString story = request.storySoFar.trimmed();
    if (!story.isEmpty()) {
        if (story.size() > kMaxContextChars) story = "…" + story.right(kMaxContextChars);
        prompt += "\nThe story so far, page by page (for context):\n" + story + "\n";
    }
    return prompt;
}

bool StoryHelper::parseResult(const QByteArray &output, QString *text, QString *error) {
    QJsonObject result;
    const QJsonDocument whole = QJsonDocument::fromJson(output.trimmed());
    if (whole.isObject()) {
        result = whole.object();
    } else {
        for (const QByteArray &line : output.split('\n')) {
            const QJsonDocument doc = QJsonDocument::fromJson(line.trimmed());
            if (doc.isObject() && doc.object()["type"].toString() == "result") result = doc.object();
        }
    }

    if (result.isEmpty() || result["type"].toString() != "result") {
        *error = "Claude Code gave an answer the app couldn't read.";
        return false;
    }
    const QString body = result["result"].toString();
    if (result["is_error"].toBool()) {
        *error = body.isEmpty() ? QString("Claude Code reported an error (%1).").arg(result["subtype"].toString()) : body;
        return false;
    }
    *text = stripWrapping(body);
    if (text->isEmpty()) {
        *error = "Claude didn't suggest anything this time. Please try again.";
        return false;
    }
    return true;
}

QString StoryHelper::storyText(const BookProject &project) {
    QStringList pages;
    for (const auto &spread : project.spreads) {
        if (spread.isCover()) continue;

        struct Snippet { int side; qreal y; QString text; };
        QList<Snippet> snippets;
        for (const auto &val : spread.canvasData["items"].toArray()) {
            const QJsonObject obj = val.toObject();
            if (obj["type"].toString() != "text") continue;
            const QString text = obj["text"].toString().trimmed();
            if (text.isEmpty()) continue;
            snippets.append({obj["x"].toDouble() < 600 ? 0 : 1, obj["y"].toDouble(), text});
        }
        if (snippets.isEmpty()) continue;
        std::sort(snippets.begin(), snippets.end(), [](const Snippet &a, const Snippet &b) {
            return a.side != b.side ? a.side < b.side : a.y < b.y;
        });

        QStringList lines;
        for (const auto &s : snippets) lines << s.text;
        pages << QString("Pages %1-%2: %3").arg(spread.leftPageNum).arg(spread.rightPageNum).arg(lines.join(" / "));
    }
    return pages.join("\n");
}

void StoryHelper::start(const Request &request, const QString &claudePath, const QString &model) {
    if (isBusy()) return;
    if (claudePath.isEmpty()) {
        emit failed(notFoundMessage());
        return;
    }

    const QString workDir = helperWorkDir();
    const QString promptFile = workDir + "/system-prompt.txt";
    QFile file(promptFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit failed("Couldn't prepare the request: " + file.errorString());
        return;
    }
    file.write(systemPrompt(request.readerAge).toUtf8());
    file.close();

    QStringList args = {
        "-p",
        "--output-format", "json",
        "--tools", "",
        "--strict-mcp-config",
        "--no-session-persistence",
        "--system-prompt-file", promptFile,
    };
    if (!model.isEmpty()) args << "--model" << model;

    QString program;
    QStringList fullArgs;
    commandFor(claudePath, args, &program, &fullArgs);

    delete m_process;
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workDir);
    m_cancelled = false;

    connect(m_process, &QProcess::finished, this, &StoryHelper::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart) return;
        m_timeout.stop();
        emit failed(notFoundMessage());
    });

    m_process->start(program, fullArgs);
    // The prompt goes through stdin so story text never needs shell quoting.
    m_process->write(buildPrompt(request).toUtf8());
    m_process->closeWriteChannel();
    m_timeout.start();
}

void StoryHelper::cancel() {
    if (!isBusy()) return;
    m_cancelled = true;
    m_timeout.stop();
    m_process->kill();
}

bool StoryHelper::isBusy() const {
    return m_process && m_process->state() != QProcess::NotRunning;
}

void StoryHelper::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_timeout.stop();
    if (m_cancelled) return;

    const QByteArray out = m_process->readAllStandardOutput();
    const QString err = QString::fromUtf8(m_process->readAllStandardError()).trimmed();

    QString text;
    QString error;
    if (parseResult(out, &text, &error)) {
        emit finished(text);
        return;
    }
    if (status == QProcess::CrashExit || (out.trimmed().isEmpty() && !err.isEmpty())) {
        error = err.isEmpty() ? QString("Claude Code stopped unexpectedly (exit code %1).").arg(exitCode) : err;
    }
    emit failed(error);
}
