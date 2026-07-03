#include "core/DiagnosticsManager.h"

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSysInfo>

#include "core/AppLogger.h"

namespace
{
struct ZipEntry
{
    QString fileName;
    QByteArray contents;
    quint32 crc = 0;
    quint32 offset = 0;
};

void appendLittleEndian16(QByteArray &target, quint16 value)
{
    target.append(static_cast<char>(value & 0xff));
    target.append(static_cast<char>((value >> 8) & 0xff));
}

void appendLittleEndian32(QByteArray &target, quint32 value)
{
    target.append(static_cast<char>(value & 0xff));
    target.append(static_cast<char>((value >> 8) & 0xff));
    target.append(static_cast<char>((value >> 16) & 0xff));
    target.append(static_cast<char>((value >> 24) & 0xff));
}

quint32 crc32(const QByteArray &data)
{
    quint32 crc = 0xffffffffU;
    for (const unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const quint32 mask = static_cast<quint32>(-(static_cast<qint32>(crc & 1U)));
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }
    return ~crc;
}

bool writeStoredZip(
    const QString &path,
    QList<ZipEntry> entries,
    QString *errorMessage
)
{
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = output.errorString();
        }
        return false;
    }

    quint32 offset = 0;
    for (ZipEntry &entry : entries) {
        const QByteArray name = entry.fileName.toUtf8();
        if (name.isEmpty() || name.size() > 0xffff || entry.contents.size() > 0xffffffffLL) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Diagnostics entry is not compatible with a standard ZIP file.");
            }
            output.cancelWriting();
            return false;
        }

        entry.crc = crc32(entry.contents);
        entry.offset = offset;

        QByteArray header;
        appendLittleEndian32(header, 0x04034b50U);
        appendLittleEndian16(header, 20);
        appendLittleEndian16(header, 0x0800);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian32(header, entry.crc);
        appendLittleEndian32(header, static_cast<quint32>(entry.contents.size()));
        appendLittleEndian32(header, static_cast<quint32>(entry.contents.size()));
        appendLittleEndian16(header, static_cast<quint16>(name.size()));
        appendLittleEndian16(header, 0);
        header.append(name);

        if (output.write(header) != header.size()
            || output.write(entry.contents) != entry.contents.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = output.errorString();
            }
            output.cancelWriting();
            return false;
        }
        offset += static_cast<quint32>(header.size() + entry.contents.size());
    }

    const quint32 centralDirectoryOffset = offset;
    for (const ZipEntry &entry : entries) {
        const QByteArray name = entry.fileName.toUtf8();
        QByteArray header;
        appendLittleEndian32(header, 0x02014b50U);
        appendLittleEndian16(header, 20);
        appendLittleEndian16(header, 20);
        appendLittleEndian16(header, 0x0800);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian32(header, entry.crc);
        appendLittleEndian32(header, static_cast<quint32>(entry.contents.size()));
        appendLittleEndian32(header, static_cast<quint32>(entry.contents.size()));
        appendLittleEndian16(header, static_cast<quint16>(name.size()));
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian16(header, 0);
        appendLittleEndian32(header, 0);
        appendLittleEndian32(header, entry.offset);
        header.append(name);

        if (output.write(header) != header.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = output.errorString();
            }
            output.cancelWriting();
            return false;
        }
        offset += static_cast<quint32>(header.size());
    }

    const quint32 centralDirectorySize = offset - centralDirectoryOffset;
    if (entries.size() > 0xffff) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Too many entries for a standard ZIP file.");
        }
        output.cancelWriting();
        return false;
    }

    QByteArray endOfDirectory;
    appendLittleEndian32(endOfDirectory, 0x06054b50U);
    appendLittleEndian16(endOfDirectory, 0);
    appendLittleEndian16(endOfDirectory, 0);
    appendLittleEndian16(endOfDirectory, static_cast<quint16>(entries.size()));
    appendLittleEndian16(endOfDirectory, static_cast<quint16>(entries.size()));
    appendLittleEndian32(endOfDirectory, centralDirectorySize);
    appendLittleEndian32(endOfDirectory, centralDirectoryOffset);
    appendLittleEndian16(endOfDirectory, 0);

    if (output.write(endOfDirectory) != endOfDirectory.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = output.errorString();
        }
        output.cancelWriting();
        return false;
    }

    if (!output.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = output.errorString();
        }
        return false;
    }

    return true;
}

QString platformSummary()
{
    return QStringLiteral("%1 | kernel %2 %3 | architecture %4")
        .arg(
            QSysInfo::prettyProductName(),
            QSysInfo::kernelType(),
            QSysInfo::kernelVersion(),
            QSysInfo::currentCpuArchitecture()
        );
}

QString diagnosticReadme()
{
    return QStringLiteral(
        "MyAppTemplate safe support package\n"
        "\n"
        "This package was created locally and was not uploaded anywhere.\n"
        "It contains: runtime-summary.txt, config-summary.json and log-excerpt.txt.\n"
        "The application does not copy config.json, license files, tokens, passwords, serials, "
        "or full home-directory paths into this package.\n"
        "\n"
        "Review the package before sharing it with support.\n"
    );
}

QString runtimeSummary(const AppConfig &config)
{
    return QStringLiteral(
        "Application: %1\n"
        "Version: %2\n"
        "Product ID: %3\n"
        "Environment: %4\n"
        "Update channel: %5\n"
        "Platform: %6\n"
        "Generated UTC: %7\n"
    ).arg(
        AppLogger::sanitize(config.appName),
        AppLogger::sanitize(config.version),
        AppLogger::sanitize(config.productId),
        AppLogger::sanitize(config.environment),
        AppLogger::sanitize(config.updateChannel),
        AppLogger::sanitize(platformSummary()),
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
    );
}
} // namespace

QString DiagnosticsManager::diagnosticsDirectoryFor(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    const QString baseDirectory = AppLogger::appDataDirectoryFor(config, baseDirectoryOverride);
    return baseDirectory.isEmpty()
        ? QString{}
        : QDir(baseDirectory).filePath(QStringLiteral("diagnostics"));
}

DiagnosticsResult DiagnosticsManager::createSafeSupportPackage(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    DiagnosticsResult result;
    const QString directory = diagnosticsDirectoryFor(config, baseDirectoryOverride);
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        result.errorMessage = QStringLiteral("Could not create the diagnostics directory.");
        return result;
    }

    const QString fileName = QStringLiteral("%1-diagnostics-%2.zip")
        .arg(
            config.appId.isEmpty() ? QStringLiteral("application") : config.appId,
            QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"))
        );
    const QString packagePath = QDir(directory).filePath(fileName);

    const QList<ZipEntry> entries{
        {QStringLiteral("README.txt"), diagnosticReadme().toUtf8()},
        {QStringLiteral("runtime-summary.txt"), runtimeSummary(config).toUtf8()},
        {QStringLiteral("config-summary.json"), safeConfigSummary(config).toUtf8()},
        {QStringLiteral("log-excerpt.txt"), safeLogExcerpt(config, baseDirectoryOverride).toUtf8()}
    };

    QString errorMessage;
    if (!writeStoredZip(packagePath, entries, &errorMessage)) {
        result.errorMessage = errorMessage.isEmpty()
            ? QStringLiteral("Could not create the diagnostics ZIP file.")
            : errorMessage;
        return result;
    }

    result.success = true;
    result.packagePath = packagePath;
    return result;
}

QString DiagnosticsManager::safeConfigSummary(const AppConfig &config)
{
    const QJsonObject summary{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("application"), QJsonObject{
            {QStringLiteral("name"), AppLogger::sanitize(config.appName)},
            {QStringLiteral("version"), AppLogger::sanitize(config.version)},
            {QStringLiteral("productId"), AppLogger::sanitize(config.productId)}
        }},
        {QStringLiteral("runtime"), QJsonObject{
            {QStringLiteral("environment"), AppLogger::sanitize(config.environment)},
            {QStringLiteral("updateChannel"), AppLogger::sanitize(config.updateChannel)},
            {QStringLiteral("theme"), AppLogger::sanitize(config.theme)},
            {QStringLiteral("loggingEnabled"), config.loggingEnabled},
            {QStringLiteral("loggingLevel"), AppLogger::sanitize(config.loggingLevel)}
        }},
        {QStringLiteral("services"), QJsonObject{
            {QStringLiteral("cdnBaseUrl"), AppLogger::sanitize(config.cdnBaseUrl)},
            {QStringLiteral("licensePortalUrl"), AppLogger::sanitize(config.licensePortalUrl)},
            {QStringLiteral("licenseApiBaseUrl"), AppLogger::sanitize(config.licenseApiBaseUrl)}
        }},
        {QStringLiteral("paths"), QJsonObject{
            {QStringLiteral("appData"), QStringLiteral("<app-data>")},
            {QStringLiteral("logs"), QStringLiteral("<app-data>/logs")}
        }}
    };

    return QString::fromUtf8(QJsonDocument(summary).toJson(QJsonDocument::Indented));
}

QString DiagnosticsManager::safeLogExcerpt(
    const AppConfig &config,
    const QString &baseDirectoryOverride
)
{
    const QString logPath = AppLogger::logFilePathFor(config, baseDirectoryOverride);
    QFile file(logPath);
    if (!file.exists()) {
        return QStringLiteral("No current application log file is available.\n");
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("The current application log file could not be read safely.\n");
    }

    const QString contents = QString::fromUtf8(file.readAll());
    const QStringList allLines = contents.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const int firstLine = qMax(0, allLines.size() - 200);

    QStringList safeLines;
    safeLines.reserve(allLines.size() - firstLine);
    for (int index = firstLine; index < allLines.size(); ++index) {
        safeLines.append(AppLogger::sanitize(allLines.at(index)));
    }

    if (safeLines.isEmpty()) {
        return QStringLiteral("The current application log file is empty.\n");
    }

    return safeLines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}
