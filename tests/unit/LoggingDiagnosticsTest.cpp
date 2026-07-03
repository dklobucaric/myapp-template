#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "core/AppLogger.h"
#include "core/ConfigManager.h"
#include "core/DiagnosticsManager.h"

class LoggingDiagnosticsTest final : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] AppConfig loadConfig(QTemporaryDir &directory) const;

private slots:
    void init();
    void cleanup();
    void disabledLoggingDoesNotCreateLogFile();
    void logLevelFiltersAndRedactsSensitiveText();
    void logRotationRetainsOnlyConfiguredArchives();
    void diagnosticsPackageExcludesSensitiveValuesAndPrivatePaths();
};

AppConfig LoggingDiagnosticsTest::loadConfig(QTemporaryDir &directory) const
{
    return ConfigManager::load(
        QStringLiteral("development"),
        directory.path()
    ).config;
}

void LoggingDiagnosticsTest::init()
{
    AppLogger::resetForTests();
}

void LoggingDiagnosticsTest::cleanup()
{
    AppLogger::resetForTests();
}

void LoggingDiagnosticsTest::disabledLoggingDoesNotCreateLogFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppConfig config = loadConfig(directory);
    QVERIFY(!config.appName.isEmpty());
    config.loggingEnabled = false;
    config.loggingLevel = QStringLiteral("debug");

    AppLogger::configure(config, directory.path());
    AppLogger::error(QStringLiteral("app"), QStringLiteral("This must not reach disk."));

    QVERIFY(!QFile::exists(AppLogger::logFilePathFor(config, directory.path())));
}

void LoggingDiagnosticsTest::logLevelFiltersAndRedactsSensitiveText()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppConfig config = loadConfig(directory);
    QVERIFY(!config.appName.isEmpty());
    config.loggingEnabled = true;
    config.loggingLevel = QStringLiteral("warning");

    AppLogger::configure(config, directory.path());
    AppLogger::info(QStringLiteral("app"), QStringLiteral("This info message is filtered."));
    AppLogger::warning(
        QStringLiteral("settings"),
        QStringLiteral("password=correct-horse token:super-secret serial=ABCD-EFGH path=%1")
            .arg(QDir::homePath())
    );

    QFile logFile(AppLogger::logFilePathFor(config, directory.path()));
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(logFile.readAll());

    QVERIFY(!text.contains(QStringLiteral("This info message is filtered.")));
    QVERIFY(text.contains(QStringLiteral("[warning]")));
    QVERIFY(text.contains(QStringLiteral("password=<redacted>")));
    QVERIFY(text.contains(QStringLiteral("token:<redacted>")));
    QVERIFY(text.contains(QStringLiteral("serial=<redacted>")));
    QVERIFY(!text.contains(QStringLiteral("correct-horse")));
    QVERIFY(!text.contains(QStringLiteral("super-secret")));
    QVERIFY(!text.contains(QDir::homePath()));
}

void LoggingDiagnosticsTest::logRotationRetainsOnlyConfiguredArchives()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppConfig config = loadConfig(directory);
    QVERIFY(!config.appName.isEmpty());
    config.loggingEnabled = true;
    config.loggingLevel = QStringLiteral("debug");

    AppLogger::configure(config, directory.path());
    AppLogger::setRotationLimitsForTests(128, 2);
    for (int index = 0; index < 8; ++index) {
        AppLogger::error(
            QStringLiteral("rotation"),
            QStringLiteral("entry %1 %2").arg(index).arg(QString(80, QLatin1Char('x')))
        );
    }

    const QString logDirectory = AppLogger::logDirectoryFor(config, directory.path());
    QVERIFY(QFile::exists(QDir(logDirectory).filePath(QStringLiteral("app.log"))));
    QVERIFY(QFile::exists(QDir(logDirectory).filePath(QStringLiteral("app.1.log"))));
    QVERIFY(QFile::exists(QDir(logDirectory).filePath(QStringLiteral("app.2.log"))));
    QVERIFY(!QFile::exists(QDir(logDirectory).filePath(QStringLiteral("app.3.log"))));
}

void LoggingDiagnosticsTest::diagnosticsPackageExcludesSensitiveValuesAndPrivatePaths()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppConfig config = loadConfig(directory);
    QVERIFY(!config.appName.isEmpty());
    config.loggingEnabled = true;
    config.loggingLevel = QStringLiteral("debug");
    config.cdnBaseUrl = QStringLiteral("https://updates.example.invalid/api?token=super-secret-token");
    config.licenseApiBaseUrl = QStringLiteral("https://license.example.invalid/api?password=not-for-support");

    AppLogger::configure(config, directory.path());
    AppLogger::warning(
        QStringLiteral("diagnostics"),
        QStringLiteral("authorization: Bearer support-token path=%1")
            .arg(QDir::homePath())
    );

    const DiagnosticsResult result = DiagnosticsManager::createSafeSupportPackage(
        config,
        directory.path()
    );
    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(QFile::exists(result.packagePath));

    QFile package(result.packagePath);
    QVERIFY(package.open(QIODevice::ReadOnly));
    const QByteArray rawPackage = package.readAll();

    QVERIFY(rawPackage.startsWith("PK"));
    QVERIFY(rawPackage.contains("runtime-summary.txt"));
    QVERIFY(rawPackage.contains("config-summary.json"));
    QVERIFY(rawPackage.contains("log-excerpt.txt"));
    QVERIFY(rawPackage.contains("<redacted>"));
    QVERIFY(rawPackage.contains("<app-data>"));
    QVERIFY(!rawPackage.contains("super-secret-token"));
    QVERIFY(!rawPackage.contains("not-for-support"));
    QVERIFY(!rawPackage.contains("support-token"));
    QVERIFY(!rawPackage.contains(QDir::homePath().toUtf8()));
}

QTEST_GUILESS_MAIN(LoggingDiagnosticsTest)

#include "LoggingDiagnosticsTest.moc"
