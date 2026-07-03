#include "core/AppLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QRegularExpression>

#include <algorithm>

namespace
{
constexpr auto kLogDirectoryName = "logs";
constexpr auto kCurrentLogFileName = "app.log";
constexpr qint64 kDefaultMaximumLogBytes = 512 * 1024;
constexpr int kDefaultArchiveCount = 4;

struct LoggerState
{
    bool enabled = false;
    AppLogger::Level minimumLevel = AppLogger::Level::Info;
    QString appDataDirectory;
    qint64 maximumLogBytes = kDefaultMaximumLogBytes;
    int archiveCount = kDefaultArchiveCount;
};

LoggerState &state()
{
    static LoggerState value;
    return value;
}

QMutex &loggerMutex()
{
    static QMutex mutex;
    return mutex;
}

int levelRank(AppLogger::Level level)
{
    switch (level) {
    case AppLogger::Level::Debug:
        return 0;
    case AppLogger::Level::Info:
        return 1;
    case AppLogger::Level::Warning:
        return 2;
    case AppLogger::Level::Error:
        return 3;
    }

    return 1;
}

AppLogger::Level levelFromText(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("debug")) {
        return AppLogger::Level::Debug;
    }
    if (normalized == QStringLiteral("warning")) {
        return AppLogger::Level::Warning;
    }
    if (normalized == QStringLiteral("error")) {
        return AppLogger::Level::Error;
    }

    return AppLogger::Level::Info;
}

QString levelText(AppLogger::Level level)
{
    switch (level) {
    case AppLogger::Level::Debug:
        return QStringLiteral("debug");
    case AppLogger::Level::Info:
        return QStringLiteral("info");
    case AppLogger::Level::Warning:
        return QStringLiteral("warning");
    case AppLogger::Level::Error:
        return QStringLiteral("error");
    }

    return QStringLiteral("info");
}

QString sanitizedText(const QString &value, const QString &appDataDirectory)
{
    QString result = value;

    const QString homePath = QDir::homePath();
    if (!homePath.isEmpty()) {
        result.replace(homePath, QStringLiteral("<home>"), Qt::CaseSensitive);
    }
    if (!appDataDirectory.isEmpty()) {
        result.replace(appDataDirectory, QStringLiteral("<app-data>"), Qt::CaseSensitive);
    }

    // Bearer credentials can include whitespace after an Authorization key,
    // so redact them before generic key-value matching.
    const QRegularExpression bearerPattern(
        QStringLiteral("(\\bBearer\\s+)([^\\s,;]+)"),
        QRegularExpression::CaseInsensitiveOption
    );
    QList<QPair<int, int>> replacements;
    QRegularExpressionMatchIterator bearerMatches = bearerPattern.globalMatch(result);
    while (bearerMatches.hasNext()) {
        const QRegularExpressionMatch match = bearerMatches.next();
        replacements.append({match.capturedStart(2), match.capturedLength(2)});
    }
    for (auto iterator = replacements.crbegin(); iterator != replacements.crend(); ++iterator) {
        result.replace(iterator->first, iterator->second, QStringLiteral("<redacted>"));
    }

    // Password- and token-shaped key-value pairs. The exact original value is
    // replaced, so accidental diagnostics do not preserve a secret.
    const QRegularExpression keyValuePattern(
        QStringLiteral("\\b(password|passwd|token|api[_-]?key|secret|serial|license(?:[_-]?key)?|authorization)\\b\\s*([:=])\\s*(\"[^\"]*\"|'[^']*'|[^\\s,;]+)"),
        QRegularExpression::CaseInsensitiveOption
    );
    replacements.clear();
    QRegularExpressionMatchIterator keyValueMatches = keyValuePattern.globalMatch(result);
    while (keyValueMatches.hasNext()) {
        const QRegularExpressionMatch match = keyValueMatches.next();
        replacements.append({match.capturedStart(3), match.capturedLength(3)});
    }
    for (auto iterator = replacements.crbegin(); iterator != replacements.crend(); ++iterator) {
        result.replace(iterator->first, iterator->second, QStringLiteral("<redacted>"));
    }

    // URLs can legally contain query strings, basic-auth credentials and
    // temporary signed URLs. Retain only the stable path before '?'.
    const QRegularExpression urlQueryPattern(QStringLiteral("(https?://[^\\s?#]+(?:/[^\\s?#]*)?)\\?[^\\s]+"));
    replacements.clear();
    QRegularExpressionMatchIterator urlMatches = urlQueryPattern.globalMatch(result);
    while (urlMatches.hasNext()) {
        const QRegularExpressionMatch match = urlMatches.next();
        replacements.append({match.capturedStart(0), match.capturedLength(0)});
    }
    for (auto iterator = replacements.crbegin(); iterator != replacements.crend(); ++iterator) {
        const QString original = result.mid(iterator->first, iterator->second);
        const int queryPosition = original.indexOf(QLatin1Char('?'));
        result.replace(
            iterator->first,
            iterator->second,
            original.left(queryPosition) + QStringLiteral("?<redacted>")
        );
    }

    const QRegularExpression basicAuthPattern(
        QStringLiteral("(https?://)([^\\s/@:]+):([^\\s/@]+)@"),
        QRegularExpression::CaseInsensitiveOption
    );
    result.replace(basicAuthPattern, QStringLiteral("\\1<redacted>@"));

    return result;
}

bool ensureDirectory(const QString &directory)
{
    return !directory.isEmpty() && QDir().mkpath(directory);
}

QString archivePath(const QString &directory, int index)
{
    return QDir(directory).filePath(QStringLiteral("app.%1.log").arg(index));
}

bool rotateIfNeeded(const LoggerState &current, qint64 incomingBytes)
{
    const QString directory = QDir(current.appDataDirectory).filePath(kLogDirectoryName);
    const QString currentLogPath = QDir(directory).filePath(kCurrentLogFileName);
    const QFileInfo currentFileInfo(currentLogPath);

    if (!currentFileInfo.exists() || currentFileInfo.size() + incomingBytes <= current.maximumLogBytes) {
        return true;
    }

    if (current.archiveCount <= 0) {
        return QFile::remove(currentLogPath);
    }

    QFile::remove(archivePath(directory, current.archiveCount));
    for (int index = current.archiveCount - 1; index >= 1; --index) {
        const QString source = archivePath(directory, index);
        const QString destination = archivePath(directory, index + 1);
        if (QFile::exists(source)) {
            QFile::remove(destination);
            if (!QFile::rename(source, destination)) {
                return false;
            }
        }
    }

    const QString firstArchive = archivePath(directory, 1);
    QFile::remove(firstArchive);
    return QFile::rename(currentLogPath, firstArchive);
}
} // namespace

void AppLogger::configure(const AppConfig &config, const QString &baseDirectoryOverride)
{
    QMutexLocker locker(&loggerMutex());
    LoggerState &current = state();
    current.enabled = config.loggingEnabled;
    current.minimumLevel = levelFromText(config.loggingLevel);
    current.appDataDirectory = appDataDirectoryFor(config, baseDirectoryOverride);
}

QString AppLogger::appDataDirectoryFor(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    if (!baseDirectoryOverride.isEmpty()) {
        return QDir::cleanPath(baseDirectoryOverride);
    }

    const QFileInfo configFile(config.userConfigPath);
    if (!configFile.absolutePath().isEmpty()) {
        return configFile.absolutePath();
    }

    return {};
}

QString AppLogger::logDirectoryFor(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    const QString baseDirectory = appDataDirectoryFor(config, baseDirectoryOverride);
    return baseDirectory.isEmpty()
        ? QString{}
        : QDir(baseDirectory).filePath(kLogDirectoryName);
}

QString AppLogger::logFilePathFor(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    const QString directory = logDirectoryFor(config, baseDirectoryOverride);
    return directory.isEmpty()
        ? QString{}
        : QDir(directory).filePath(kCurrentLogFileName);
}

QString AppLogger::logDirectory()
{
    QMutexLocker locker(&loggerMutex());
    const QString baseDirectory = state().appDataDirectory;
    return baseDirectory.isEmpty()
        ? QString{}
        : QDir(baseDirectory).filePath(kLogDirectoryName);
}

QString AppLogger::logFilePath()
{
    const QString directory = logDirectory();
    return directory.isEmpty()
        ? QString{}
        : QDir(directory).filePath(kCurrentLogFileName);
}

bool AppLogger::isEnabled(Level level)
{
    QMutexLocker locker(&loggerMutex());
    const LoggerState &current = state();
    return current.enabled && levelRank(level) >= levelRank(current.minimumLevel);
}

void AppLogger::debug(const QString &category, const QString &message)
{
    write(Level::Debug, category, message);
}

void AppLogger::info(const QString &category, const QString &message)
{
    write(Level::Info, category, message);
}

void AppLogger::warning(const QString &category, const QString &message)
{
    write(Level::Warning, category, message);
}

void AppLogger::error(const QString &category, const QString &message)
{
    write(Level::Error, category, message);
}

QString AppLogger::sanitize(const QString &text)
{
    QMutexLocker locker(&loggerMutex());
    return sanitizedText(text, state().appDataDirectory);
}

void AppLogger::resetForTests()
{
    QMutexLocker locker(&loggerMutex());
    state() = LoggerState{};
}

void AppLogger::setRotationLimitsForTests(qint64 maxBytes, int archiveCount)
{
    QMutexLocker locker(&loggerMutex());
    state().maximumLogBytes = std::max<qint64>(64, maxBytes);
    state().archiveCount = std::max(0, archiveCount);
}

void AppLogger::write(Level level, const QString &category, const QString &message)
{
    QMutexLocker locker(&loggerMutex());
    const LoggerState current = state();
    if (!current.enabled || levelRank(level) < levelRank(current.minimumLevel)) {
        return;
    }

    const QString directory = QDir(current.appDataDirectory).filePath(kLogDirectoryName);
    if (!ensureDirectory(directory)) {
        return;
    }

    const QString safeCategory = sanitizedText(category.trimmed(), current.appDataDirectory);
    const QString safeMessage = sanitizedText(message, current.appDataDirectory);
    const QString line = QStringLiteral("%1 [%2] [%3] %4\n")
        .arg(
            QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
            levelText(level),
            safeCategory.isEmpty() ? QStringLiteral("app") : safeCategory,
            safeMessage
        );
    const QByteArray encodedLine = line.toUtf8();

    if (!rotateIfNeeded(current, encodedLine.size())) {
        return;
    }

    QFile file(QDir(directory).filePath(kCurrentLogFileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    file.write(encodedLine);
}
