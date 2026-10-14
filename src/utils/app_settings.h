#pragma once

#include <QString>

// App-wide preferences, kept in settings.ini next to the books.
class AppSettings {
public:
    enum class HelperBackend { Off, ClaudeCode };

    static HelperBackend helperBackend();
    static void setHelperBackend(HelperBackend backend);

    // Empty means "find it automatically".
    static QString claudePath();
    static void setClaudePath(const QString &path);

    // A Claude Code model alias ("haiku", "sonnet", "opus", "fable"); empty uses Claude Code's default.
    static QString claudeModel();
    static void setClaudeModel(const QString &model);

    // "3-5", "5-8" or "8-11": tunes the helper's vocabulary.
    static QString readerAge();
    static void setReaderAge(const QString &age);

    // Tests point this at a scratch file.
    static void setFilePath(const QString &path);
};
