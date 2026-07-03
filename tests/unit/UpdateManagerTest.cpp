#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "core/AppLogger.h"
#include "core/ConfigManager.h"
#include "updates/UpdateManager.h"

class UpdateManagerTest final : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] AppConfig loadConfig(QTemporaryDir &directory) const;
    [[nodiscard]] QByteArray validManifest(const AppConfig &config, const QString &version = QStringLiteral("0.1.5")) const;

private slots:
    void init();
    void updateAvailableManifest();
    void currentVersionManifest();
    void malformedManifestFailsSafely();
    void missingRequiredFieldFailsSafely();
    void incompatiblePlatformIsReported();
    void semanticVersionComparison();
    void linuxDistributionNamesUseLinuxArtifactFamily();
    void networkFailurePersistsSafeState();
    void automaticChecksRescheduleWhenSettingsChange();
};

AppConfig UpdateManagerTest::loadConfig(QTemporaryDir &directory) const
{
    return ConfigManager::load(QStringLiteral("development"), directory.path()).config;
}

QByteArray UpdateManagerTest::validManifest(const AppConfig &config, const QString &version) const
{
    return QString::fromUtf8(R"({
      "schemaVersion": 1,
      "productId": "%1",
      "channel": "development",
      "platform": "linux",
      "architecture": "x64",
      "version": "%2",
      "publishedAt": "2026-07-03T20:00:00Z",
      "artifactFileName": "MyAppTemplate-%2.AppImage",
      "artifactUrl": "https://updates.example.invalid/MyAppTemplate-%2.AppImage",
      "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
      "sizeBytes": 42,
      "releaseNotesUrl": "https://updates.example.invalid/release-notes/%2.json",
      "mandatory": false
    })").arg(config.productId, version).toUtf8();
}

void UpdateManagerTest::init()
{
    AppLogger::resetForTests();
}

void UpdateManagerTest::updateAvailableManifest()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    const UpdateCheckResult result = UpdateManager::evaluateManifest(
        config, validManifest(config), QStringLiteral("https://updates.example.invalid/manifest.json"),
        QStringLiteral("linux"), QStringLiteral("x64")
    );
    QCOMPARE(result.status, UpdateStatus::UpdateAvailable);
    QCOMPARE(result.availableVersion, QStringLiteral("0.1.5"));
    QVERIFY(result.message.contains(QStringLiteral("0.1.5")));
}

void UpdateManagerTest::currentVersionManifest()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    const UpdateCheckResult result = UpdateManager::evaluateManifest(
        config, validManifest(config, config.version), QStringLiteral("https://updates.example.invalid/manifest.json"),
        QStringLiteral("linux"), QStringLiteral("x64")
    );
    QCOMPARE(result.status, UpdateStatus::UpToDate);
}

void UpdateManagerTest::malformedManifestFailsSafely()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);
    const UpdateCheckResult result = UpdateManager::evaluateManifest(
        config, QByteArrayLiteral("{ invalid json"), QStringLiteral("https://updates.example.invalid/manifest.json"),
        QStringLiteral("linux"), QStringLiteral("x64")
    );
    QCOMPARE(result.status, UpdateStatus::InvalidManifest);
}

void UpdateManagerTest::missingRequiredFieldFailsSafely()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);
    QByteArray manifest = validManifest(config);
    manifest.replace("\"sha256\": \"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\",\n", "");
    const UpdateCheckResult result = UpdateManager::evaluateManifest(
        config, manifest, QStringLiteral("https://updates.example.invalid/manifest.json"),
        QStringLiteral("linux"), QStringLiteral("x64")
    );
    QCOMPARE(result.status, UpdateStatus::InvalidManifest);
    QVERIFY(result.message.contains(QStringLiteral("sha256")));
}

void UpdateManagerTest::incompatiblePlatformIsReported()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);
    const UpdateCheckResult result = UpdateManager::evaluateManifest(
        config, validManifest(config), QStringLiteral("https://updates.example.invalid/manifest.json"),
        QStringLiteral("windows"), QStringLiteral("x64")
    );
    QCOMPARE(result.status, UpdateStatus::UnsupportedPlatform);
}

void UpdateManagerTest::semanticVersionComparison()
{
    QString error;
    QCOMPARE(UpdateManager::compareSemanticVersions(QStringLiteral("0.1.4"), QStringLiteral("0.1.5"), &error), -1);
    QVERIFY(error.isEmpty());
    QCOMPARE(UpdateManager::compareSemanticVersions(QStringLiteral("1.10.0"), QStringLiteral("1.2.9"), &error), 1);
    QCOMPARE(UpdateManager::compareSemanticVersions(QStringLiteral("1.2.3-beta.2"), QStringLiteral("1.2.3-beta.11"), &error), -1);
    QCOMPARE(UpdateManager::compareSemanticVersions(QStringLiteral("1.2.3-beta"), QStringLiteral("1.2.3"), &error), -1);
    QCOMPARE(UpdateManager::compareSemanticVersions(QStringLiteral("1.2.3"), QStringLiteral("1.2.3"), &error), 0);
    const int invalidResult = UpdateManager::compareSemanticVersions(
        QStringLiteral("not-a-version"), QStringLiteral("1.2.3"), &error
    );
    QCOMPARE(invalidResult, 0);
    QVERIFY(!error.isEmpty());
}

void UpdateManagerTest::linuxDistributionNamesUseLinuxArtifactFamily()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    QCOMPARE(UpdateManager::normalizedPlatform(QStringLiteral("linuxmint")), QStringLiteral("linux"));
    QCOMPARE(UpdateManager::normalizedPlatform(QStringLiteral("ubuntu")), QStringLiteral("linux"));
    QCOMPARE(UpdateManager::normalizedPlatform(QStringLiteral("fedora")), QStringLiteral("linux"));

    const QUrl manifestUrl = UpdateManager::manifestUrlFor(
        config,
        QStringLiteral("linuxmint"),
        QStringLiteral("x86_64")
    );
    QCOMPARE(
        manifestUrl.path(),
        QStringLiteral("/products/myapp-template/development/linux/x64/manifest.json")
    );
}

void UpdateManagerTest::networkFailurePersistsSafeState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    AppConfig config = loadConfig(directory);
    config.cdnBaseUrl = QStringLiteral("http://127.0.0.1:9/products/myapp-template?token=super-secret");
    config.loggingEnabled = true;
    config.loggingLevel = QStringLiteral("debug");
    AppLogger::configure(config, directory.path());

    UpdateManager manager(config, nullptr, QStringLiteral("linux"), QStringLiteral("x64"), directory.path());
    QSignalSpy completed(&manager, &UpdateManager::checkCompleted);
    manager.checkNow();
    QVERIFY2(completed.wait(5000), "Network failure did not complete promptly.");

    const UpdateCheckResult result = qvariant_cast<UpdateCheckResult>(completed.takeFirst().at(0));
    QCOMPARE(result.status, UpdateStatus::NetworkError);
    QVERIFY(!result.manifestUrl.contains(QStringLiteral("super-secret")));

    const QString statePath = UpdateManager::updateStatePathFor(config, directory.path());
    QFile stateFile(statePath);
    QVERIFY(stateFile.open(QIODevice::ReadOnly));
    const QByteArray state = stateFile.readAll();
    QVERIFY(state.contains("network-error"));
    QVERIFY(!state.contains("super-secret"));
}


void UpdateManagerTest::automaticChecksRescheduleWhenSettingsChange()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppConfig config = loadConfig(directory);
    config.cdnBaseUrl = QStringLiteral("http://127.0.0.1:9/products/myapp-template");
    config.automaticUpdateChecks = false;
    config.updateCheckIntervalMinutes = 5;

    const QString statePath = UpdateManager::updateStatePathFor(config, directory.path());
    QFile stateFile(statePath);
    QVERIFY(stateFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QJsonObject state{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("status"), QStringLiteral("network-error")},
        {QStringLiteral("lastCheckedAt"), QDateTime::currentDateTimeUtc().addSecs(-6 * 60).toString(Qt::ISODate)}
    };
    QVERIFY(stateFile.write(QJsonDocument(state).toJson()) > 0);
    stateFile.close();

    UpdateManager manager(config, nullptr, QStringLiteral("linux"), QStringLiteral("x64"), directory.path());
    QSignalSpy started(&manager, &UpdateManager::checkStarted);

    manager.startAutomaticChecks();
    QTest::qWait(50);
    QCOMPARE(started.count(), 0);

    config.automaticUpdateChecks = true;
    manager.setConfig(config);
    QVERIFY2(started.wait(1000), "Automatic check did not start after enabling a due interval.");
    manager.stopAutomaticChecks();
}

QTEST_GUILESS_MAIN(UpdateManagerTest)

#include "UpdateManagerTest.moc"
