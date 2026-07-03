#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "core/AppLogger.h"
#include "core/ConfigManager.h"
#include "licensing/LicenseManager.h"

class LicenseManagerTest final : public QObject
{
    Q_OBJECT

private:
    [[nodiscard]] AppConfig loadConfig(QTemporaryDir &directory) const;
    [[nodiscard]] QByteArray validResponse(const AppConfig &config, const QString &status = QStringLiteral("active")) const;
    void writeCachedActiveState(const AppConfig &config, const QString &directory, const QDateTime &serverTime) const;

private slots:
    void init();
    void activeLicenseResponseIsValidated();
    void invalidProductIsRejected();
    void malformedResponseFailsSafely();
    void serverExpiredStateIsPreserved();
    void networkFailureUsesOfflineGraceAndRedactsState();
    void localClockRollbackDoesNotLockCachedLicense();
    void automaticChecksStartWhenDue();
};

AppConfig LicenseManagerTest::loadConfig(QTemporaryDir &directory) const
{
    return ConfigManager::load(QStringLiteral("development"), directory.path()).config;
}

QByteArray LicenseManagerTest::validResponse(const AppConfig &config, const QString &status) const
{
    return QString::fromUtf8(R"({
      "schemaVersion": 1,
      "success": true,
      "data": {
        "licenseId": "lic_test_000001",
        "serial": "DDLA-7J4K-P9Q2-M8X5",
        "productId": "%1",
        "status": "%2",
        "planName": "Developer Annual",
        "durationDays": 365,
        "remainingDays": 365,
        "activeDeviceCount": 1,
        "deviceLimit": 3,
        "features": ["updates", "diagnostics"],
        "issuedAt": "2026-07-03T16:00:00Z",
        "serverTime": "2026-07-03T16:00:00Z",
        "expiresAt": "2027-07-03T23:59:59Z",
        "graceUntil": "2027-07-17T23:59:59Z",
        "nextOnlineCheckAfter": "2026-07-03T17:00:00Z"
      }
    })").arg(config.productId, status).toUtf8();
}

void LicenseManagerTest::writeCachedActiveState(
    const AppConfig &config,
    const QString &directory,
    const QDateTime &serverTime
) const
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QJsonObject state{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("status"), QStringLiteral("active")},
        {QStringLiteral("message"), QStringLiteral("License is active.")},
        {QStringLiteral("endpointUrl"), QStringLiteral("http://127.0.0.1:9/api/v1/license-status.json")},
        {QStringLiteral("checkedAt"), now.addSecs(-6 * 60).toString(Qt::ISODate)},
        {QStringLiteral("lastVerifiedAt"), now.addSecs(-6 * 60).toString(Qt::ISODate)},
        {QStringLiteral("entitlement"), QJsonObject{
            {QStringLiteral("licenseId"), QStringLiteral("lic_test_000001")},
            {QStringLiteral("serial"), QStringLiteral("••••-••••-••••-M8X5")},
            {QStringLiteral("productId"), config.productId},
            {QStringLiteral("serverStatus"), QStringLiteral("active")},
            {QStringLiteral("planName"), QStringLiteral("Developer Annual")},
            {QStringLiteral("durationDays"), 365},
            {QStringLiteral("remainingDays"), 365},
            {QStringLiteral("deviceLimit"), 3},
            {QStringLiteral("activeDeviceCount"), 1},
            {QStringLiteral("features"), QJsonArray{QStringLiteral("updates")}},
            {QStringLiteral("issuedAt"), now.addDays(-1).toString(Qt::ISODate)},
            {QStringLiteral("serverTime"), serverTime.toString(Qt::ISODate)},
            {QStringLiteral("expiresAt"), now.addDays(365).toString(Qt::ISODate)},
            {QStringLiteral("graceUntil"), now.addDays(7).toString(Qt::ISODate)},
            {QStringLiteral("nextOnlineCheckAfter"), now.addSecs(-1).toString(Qt::ISODate)}
        }}
    };

    QFile file(LicenseManager::licenseStatePathFor(config, directory));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(file.write(QJsonDocument(state).toJson()) > 0);
}

void LicenseManagerTest::init()
{
    AppLogger::resetForTests();
}

void LicenseManagerTest::activeLicenseResponseIsValidated()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    const LicenseCheckResult result = LicenseManager::evaluateResponse(
        config,
        validResponse(config),
        QStringLiteral("https://license.example.invalid/api/v1/license-status.json")
    );

    QCOMPARE(result.status, LicenseStatus::Active);
    QCOMPARE(result.entitlement.licenseId, QStringLiteral("lic_test_000001"));
    QCOMPARE(result.entitlement.serial, QStringLiteral("DDLA-7J4K-P9Q2-M8X5"));
    QCOMPARE(result.entitlement.deviceLimit, 3);
    QCOMPARE(result.entitlement.activeDeviceCount, 1);
    QCOMPARE(result.entitlement.durationDays, 365);
    QCOMPARE(LicenseManager::redactedSerial(result.entitlement.serial), QStringLiteral("••••-••••-••••-M8X5"));
}

void LicenseManagerTest::invalidProductIsRejected()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    QByteArray payload = validResponse(config);
    payload.replace(config.productId.toUtf8(), QByteArrayLiteral("hr.ddlab.wrong-product"));
    const LicenseCheckResult result = LicenseManager::evaluateResponse(
        config,
        payload,
        QStringLiteral("https://license.example.invalid/api/v1/license-status.json")
    );

    QCOMPARE(result.status, LicenseStatus::InvalidResponse);
    QVERIFY(result.message.contains(QStringLiteral("product ID")));
}

void LicenseManagerTest::malformedResponseFailsSafely()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    const LicenseCheckResult result = LicenseManager::evaluateResponse(
        config,
        QByteArrayLiteral("{ invalid json"),
        QStringLiteral("https://license.example.invalid/api/v1/license-status.json")
    );

    QCOMPARE(result.status, LicenseStatus::InvalidResponse);
}

void LicenseManagerTest::serverExpiredStateIsPreserved()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const AppConfig config = loadConfig(directory);

    const LicenseCheckResult result = LicenseManager::evaluateResponse(
        config,
        validResponse(config, QStringLiteral("expired")),
        QStringLiteral("https://license.example.invalid/api/v1/license-status.json")
    );

    QCOMPARE(result.status, LicenseStatus::Expired);
    QCOMPARE(result.entitlement.serverStatus, QStringLiteral("expired"));
}

void LicenseManagerTest::networkFailureUsesOfflineGraceAndRedactsState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    AppConfig config = loadConfig(directory);
    config.licenseApiBaseUrl = QStringLiteral("http://127.0.0.1:9/api/v1?token=super-secret-token");
    config.loggingEnabled = true;
    config.loggingLevel = QStringLiteral("debug");
    AppLogger::configure(config, directory.path());

    writeCachedActiveState(config, directory.path(), QDateTime::currentDateTimeUtc().addSecs(-60));

    LicenseManager manager(config, nullptr, directory.path());
    QSignalSpy completed(&manager, &LicenseManager::checkCompleted);
    manager.checkNow();
    QVERIFY2(completed.wait(5000), "Network failure did not complete promptly.");

    const LicenseCheckResult result = qvariant_cast<LicenseCheckResult>(completed.takeFirst().at(0));
    QCOMPARE(result.status, LicenseStatus::OfflineGrace);
    QVERIFY(!result.endpointUrl.contains(QStringLiteral("super-secret-token")));

    QFile state(LicenseManager::licenseStatePathFor(config, directory.path()));
    QVERIFY(state.open(QIODevice::ReadOnly));
    const QByteArray raw = state.readAll();
    QVERIFY(raw.contains("offline-grace"));
    QVERIFY(!raw.contains("super-secret-token"));
    QVERIFY(!raw.contains("DDLA-7J4K-P9Q2-M8X5"));
}

void LicenseManagerTest::localClockRollbackDoesNotLockCachedLicense()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    AppConfig config = loadConfig(directory);
    config.licenseApiBaseUrl = QStringLiteral("http://127.0.0.1:9/api/v1");

    writeCachedActiveState(config, directory.path(), QDateTime::currentDateTimeUtc().addDays(1));

    LicenseManager manager(config, nullptr, directory.path());
    QSignalSpy completed(&manager, &LicenseManager::checkCompleted);
    manager.checkNow();
    QVERIFY2(completed.wait(5000), "Clock rollback case did not complete promptly.");

    const LicenseCheckResult result = qvariant_cast<LicenseCheckResult>(completed.takeFirst().at(0));
    QCOMPARE(result.status, LicenseStatus::OfflineGrace);
}

void LicenseManagerTest::automaticChecksStartWhenDue()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    AppConfig config = loadConfig(directory);
    config.licenseApiBaseUrl = QStringLiteral("http://127.0.0.1:9/api/v1");
    config.automaticLicenseChecks = false;
    config.licenseCheckIntervalMinutes = 5;
    writeCachedActiveState(config, directory.path(), QDateTime::currentDateTimeUtc().addSecs(-60));

    LicenseManager manager(config, nullptr, directory.path());
    QSignalSpy started(&manager, &LicenseManager::checkStarted);

    manager.startAutomaticChecks();
    QTest::qWait(50);
    QCOMPARE(started.count(), 0);

    config.automaticLicenseChecks = true;
    manager.setConfig(config);
    QVERIFY2(started.wait(1000), "Automatic license check did not start after enabling a due interval.");
    manager.stopAutomaticChecks();
}

QTEST_GUILESS_MAIN(LicenseManagerTest)

#include "LicenseManagerTest.moc"
