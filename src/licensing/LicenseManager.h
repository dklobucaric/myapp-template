#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "core/ConfigManager.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

/**
 * Server-authoritative license outcomes. The client displays these states but
 * does not lock the application based only on its local clock.
 */
enum class LicenseStatus {
    NotChecked,
    Checking,
    Active,
    Grace,
    Expired,
    Revoked,
    OfflineGrace,
    VerificationRequired,
    NetworkError,
    InvalidResponse,
    Disabled
};

/** A validated entitlement response returned by the license service. */
struct LicenseEntitlement
{
    int schemaVersion = 0;
    QString licenseId;
    QString serial;
    QString productId;
    QString serverStatus;
    QString planName;
    int durationDays = 0;
    int remainingDays = 0;
    int deviceLimit = 0;
    int activeDeviceCount = 0;
    QStringList features;
    QDateTime issuedAtUtc;
    QDateTime serverTimeUtc;
    QDateTime expiresAtUtc;
    QDateTime graceUntilUtc;
    QDateTime nextOnlineCheckAfterUtc;
};

struct LicenseCheckResult
{
    LicenseStatus status = LicenseStatus::NotChecked;
    QString message;
    QString endpointUrl;
    QDateTime checkedAtUtc;
    QDateTime lastVerifiedAtUtc;
    LicenseEntitlement entitlement;

    [[nodiscard]] bool isError() const;
    [[nodiscard]] bool isTerminal() const;
};

/**
 * License status client for the template's mock/real license service.
 *
 * The server owns issuance, duration, expiry, revocation and device limits.
 * A serial is an identifier only; it never encodes product duration or seats.
 * During a network failure the client may show cached offline grace, but it
 * never performs a local-clock-only lockout.
 */
class LicenseManager final : public QObject
{
    Q_OBJECT

public:
    explicit LicenseManager(
        const AppConfig &config,
        QObject *parent = nullptr,
        QString appDataDirectoryOverride = {}
    );

    void setConfig(const AppConfig &config);
    [[nodiscard]] const AppConfig &config() const;
    [[nodiscard]] bool isChecking() const;
    [[nodiscard]] LicenseCheckResult lastResult() const;

    void checkNow();
    void cancel();

    void startAutomaticChecks();
    void stopAutomaticChecks();
    [[nodiscard]] bool automaticChecksRunning() const;
    [[nodiscard]] qint64 millisecondsUntilNextAutomaticCheck() const;
    [[nodiscard]] bool shouldCheckAutomatically() const;

    [[nodiscard]] static QUrl licenseStatusUrlFor(const AppConfig &config);
    [[nodiscard]] static QString licenseStatePathFor(
        const AppConfig &config,
        const QString &appDataDirectoryOverride = {}
    );
    [[nodiscard]] static LicenseCheckResult evaluateResponse(
        const AppConfig &config,
        const QByteArray &payload,
        const QString &endpointUrl
    );
    [[nodiscard]] static QString statusText(LicenseStatus status);
    [[nodiscard]] static QString redactedSerial(const QString &serial);

signals:
    void checkStarted();
    void checkCompleted(const LicenseCheckResult &result);

private:
    void complete(LicenseCheckResult result);
    void scheduleAutomaticCheck();
    [[nodiscard]] bool licensesEnabledByProfile() const;
    [[nodiscard]] LicenseCheckResult offlineResultFor(const QString &networkMessage) const;
    void persistState(const LicenseCheckResult &result) const;
    [[nodiscard]] LicenseCheckResult readPersistedState() const;

    AppConfig m_config;
    QNetworkAccessManager *m_network = nullptr;
    QTimer *m_automaticCheckTimer = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_appDataDirectoryOverride;
    LicenseCheckResult m_lastResult;
    bool m_automaticChecksStarted = false;
};

Q_DECLARE_METATYPE(LicenseCheckResult)
