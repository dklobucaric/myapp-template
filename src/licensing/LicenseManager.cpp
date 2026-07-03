#include "licensing/LicenseManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <limits>
#include <utility>

#include "core/AppLogger.h"

namespace
{
constexpr int kLicenseStateSchemaVersion = 1;
constexpr auto kStateFileName = "license-state.json";

QString appDataDirectory(const AppConfig &config, const QString &overrideDirectory)
{
    if (!overrideDirectory.isEmpty()) {
        return QDir::cleanPath(overrideDirectory);
    }

    const QFileInfo configFile(config.userConfigPath);
    return configFile.absolutePath();
}

QString safeUrlText(const QUrl &url)
{
    return AppLogger::sanitize(url.toString(QUrl::FullyEncoded));
}

bool isHttpUrl(const QUrl &url)
{
    return url.isValid()
        && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))
        && !url.host().isEmpty();
}

QString requiredString(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("License response field '%1' must be a non-empty string.").arg(key);
        }
        return {};
    }
    return value.toString().trimmed();
}

int requiredNonNegativeInteger(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble() || value.toInt(-1) < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("License response field '%1' must be a non-negative integer.").arg(key);
        }
        return -1;
    }
    return value.toInt();
}

int requiredPositiveInteger(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const int value = requiredNonNegativeInteger(object, key, errorMessage);
    if (value == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("License response field '%1' must be greater than zero.").arg(key);
        }
        return -1;
    }
    return value;
}

QDateTime requiredUtcDateTime(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const QString value = requiredString(object, key, errorMessage);
    if (value.isEmpty()) {
        return {};
    }

    const QDateTime parsed = QDateTime::fromString(value, Qt::ISODate);
    if (!parsed.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("License response field '%1' must be an ISO-8601 timestamp.").arg(key);
        }
        return {};
    }
    return parsed.toUTC();
}

QStringList requiredStringArray(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isArray()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("License response field '%1' must be an array.").arg(key);
        }
        return {};
    }

    QStringList values;
    for (const QJsonValue &item : value.toArray()) {
        if (!item.isString() || item.toString().trimmed().isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("License response field '%1' contains an invalid feature.").arg(key);
            }
            return {};
        }
        values.append(item.toString().trimmed());
    }
    return values;
}

bool isServerStatus(const QString &value)
{
    return value == QStringLiteral("active")
        || value == QStringLiteral("grace")
        || value == QStringLiteral("expired")
        || value == QStringLiteral("revoked");
}


QString statusKey(LicenseStatus status)
{
    switch (status) {
    case LicenseStatus::NotChecked: return QStringLiteral("not-checked");
    case LicenseStatus::Checking: return QStringLiteral("checking");
    case LicenseStatus::Active: return QStringLiteral("active");
    case LicenseStatus::Grace: return QStringLiteral("grace");
    case LicenseStatus::Expired: return QStringLiteral("expired");
    case LicenseStatus::Revoked: return QStringLiteral("revoked");
    case LicenseStatus::OfflineGrace: return QStringLiteral("offline-grace");
    case LicenseStatus::VerificationRequired: return QStringLiteral("verification-required");
    case LicenseStatus::NetworkError: return QStringLiteral("network-error");
    case LicenseStatus::InvalidResponse: return QStringLiteral("invalid-response");
    case LicenseStatus::Disabled: return QStringLiteral("disabled");
    }
    return QStringLiteral("not-checked");
}

LicenseStatus statusFromText(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("active")) return LicenseStatus::Active;
    if (normalized == QStringLiteral("grace")) return LicenseStatus::Grace;
    if (normalized == QStringLiteral("expired")) return LicenseStatus::Expired;
    if (normalized == QStringLiteral("revoked")) return LicenseStatus::Revoked;
    if (normalized == QStringLiteral("offline-grace")) return LicenseStatus::OfflineGrace;
    if (normalized == QStringLiteral("verification-required")) return LicenseStatus::VerificationRequired;
    if (normalized == QStringLiteral("network-error")) return LicenseStatus::NetworkError;
    if (normalized == QStringLiteral("invalid-response")) return LicenseStatus::InvalidResponse;
    if (normalized == QStringLiteral("disabled")) return LicenseStatus::Disabled;
    if (normalized == QStringLiteral("checking")) return LicenseStatus::Checking;
    return LicenseStatus::NotChecked;
}

LicenseStatus statusForServerValue(const QString &value)
{
    if (value == QStringLiteral("active")) return LicenseStatus::Active;
    if (value == QStringLiteral("grace")) return LicenseStatus::Grace;
    if (value == QStringLiteral("expired")) return LicenseStatus::Expired;
    if (value == QStringLiteral("revoked")) return LicenseStatus::Revoked;
    return LicenseStatus::InvalidResponse;
}

QString utcText(const QDateTime &value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODate) : QString{};
}

QDateTime parseUtc(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        return {};
    }
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
    return parsed.isValid() ? parsed.toUTC() : QDateTime{};
}

QJsonObject entitlementToJson(const LicenseEntitlement &entitlement)
{
    QJsonArray features;
    for (const QString &feature : entitlement.features) {
        features.append(feature);
    }

    return {
        {QStringLiteral("licenseId"), entitlement.licenseId},
        {QStringLiteral("serial"), LicenseManager::redactedSerial(entitlement.serial)},
        {QStringLiteral("productId"), entitlement.productId},
        {QStringLiteral("serverStatus"), entitlement.serverStatus},
        {QStringLiteral("planName"), entitlement.planName},
        {QStringLiteral("durationDays"), entitlement.durationDays},
        {QStringLiteral("remainingDays"), entitlement.remainingDays},
        {QStringLiteral("deviceLimit"), entitlement.deviceLimit},
        {QStringLiteral("activeDeviceCount"), entitlement.activeDeviceCount},
        {QStringLiteral("features"), features},
        {QStringLiteral("issuedAt"), utcText(entitlement.issuedAtUtc)},
        {QStringLiteral("serverTime"), utcText(entitlement.serverTimeUtc)},
        {QStringLiteral("expiresAt"), utcText(entitlement.expiresAtUtc)},
        {QStringLiteral("graceUntil"), utcText(entitlement.graceUntilUtc)},
        {QStringLiteral("nextOnlineCheckAfter"), utcText(entitlement.nextOnlineCheckAfterUtc)}
    };
}

LicenseEntitlement entitlementFromJson(const QJsonObject &object)
{
    LicenseEntitlement entitlement;
    entitlement.licenseId = object.value(QStringLiteral("licenseId")).toString();
    entitlement.serial = object.value(QStringLiteral("serial")).toString();
    entitlement.productId = object.value(QStringLiteral("productId")).toString();
    entitlement.serverStatus = object.value(QStringLiteral("serverStatus")).toString();
    entitlement.planName = object.value(QStringLiteral("planName")).toString();
    entitlement.durationDays = object.value(QStringLiteral("durationDays")).toInt();
    entitlement.remainingDays = object.value(QStringLiteral("remainingDays")).toInt();
    entitlement.deviceLimit = object.value(QStringLiteral("deviceLimit")).toInt();
    entitlement.activeDeviceCount = object.value(QStringLiteral("activeDeviceCount")).toInt();
    for (const QJsonValue &feature : object.value(QStringLiteral("features")).toArray()) {
        if (feature.isString()) {
            entitlement.features.append(feature.toString());
        }
    }
    entitlement.issuedAtUtc = parseUtc(object, QStringLiteral("issuedAt"));
    entitlement.serverTimeUtc = parseUtc(object, QStringLiteral("serverTime"));
    entitlement.expiresAtUtc = parseUtc(object, QStringLiteral("expiresAt"));
    entitlement.graceUntilUtc = parseUtc(object, QStringLiteral("graceUntil"));
    entitlement.nextOnlineCheckAfterUtc = parseUtc(object, QStringLiteral("nextOnlineCheckAfter"));
    return entitlement;
}

bool writeJsonAtomically(const QString &path, const QJsonObject &object)
{
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.dir().absolutePath())) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }

    return file.commit();
}
} // namespace

bool LicenseCheckResult::isError() const
{
    return status == LicenseStatus::NetworkError
        || status == LicenseStatus::InvalidResponse
        || status == LicenseStatus::VerificationRequired;
}

bool LicenseCheckResult::isTerminal() const
{
    return status != LicenseStatus::NotChecked && status != LicenseStatus::Checking;
}

LicenseManager::LicenseManager(
    const AppConfig &config,
    QObject *parent,
    QString appDataDirectoryOverride
)
    : QObject(parent)
    , m_config(config)
    , m_network(new QNetworkAccessManager(this))
    , m_automaticCheckTimer(new QTimer(this))
    , m_appDataDirectoryOverride(std::move(appDataDirectoryOverride))
    , m_lastResult(readPersistedState())
{
    qRegisterMetaType<LicenseCheckResult>("LicenseCheckResult");
    m_automaticCheckTimer->setSingleShot(true);
    connect(m_automaticCheckTimer, &QTimer::timeout, this, [this] {
        checkNow();
    });
}

void LicenseManager::setConfig(const AppConfig &config)
{
    m_config = config;
    if (m_automaticChecksStarted) {
        scheduleAutomaticCheck();
    }
}

const AppConfig &LicenseManager::config() const
{
    return m_config;
}

bool LicenseManager::isChecking() const
{
    return m_reply != nullptr;
}

LicenseCheckResult LicenseManager::lastResult() const
{
    return m_lastResult;
}

void LicenseManager::checkNow()
{
    if (isChecking()) {
        return;
    }

    if (!licensesEnabledByProfile()) {
        LicenseCheckResult result;
        result.status = LicenseStatus::Disabled;
        result.message = QStringLiteral("License checks are disabled by the active product profile.");
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
        complete(result);
        return;
    }

    const QUrl endpoint = licenseStatusUrlFor(m_config);
    if (!isHttpUrl(endpoint)) {
        LicenseCheckResult result;
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("The configured license API URL is not a valid HTTP endpoint.");
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
        complete(result);
        return;
    }

    m_lastResult = {};
    m_lastResult.status = LicenseStatus::Checking;
    m_lastResult.endpointUrl = safeUrlText(endpoint);
    emit checkStarted();

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MyAppTemplate-LicenseManager"));
    request.setRawHeader("Accept", "application/json");

    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this, endpoint] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;

        if (reply == nullptr) {
            return;
        }

        LicenseCheckResult result;
        if (reply->error() != QNetworkReply::NoError) {
            result = offlineResultFor(
                QStringLiteral("Could not reach the license server: %1").arg(reply->errorString())
            );
            result.endpointUrl = safeUrlText(endpoint);
            result.checkedAtUtc = QDateTime::currentDateTimeUtc();
        } else {
            result = evaluateResponse(
                m_config,
                reply->readAll(),
                safeUrlText(endpoint)
            );
        }
        reply->deleteLater();
        complete(result);
    });
}

void LicenseManager::cancel()
{
    if (m_reply != nullptr) {
        m_reply->abort();
    }
}

void LicenseManager::startAutomaticChecks()
{
    m_automaticChecksStarted = true;
    scheduleAutomaticCheck();
}

void LicenseManager::stopAutomaticChecks()
{
    m_automaticChecksStarted = false;
    m_automaticCheckTimer->stop();
}

bool LicenseManager::automaticChecksRunning() const
{
    return m_automaticChecksStarted && m_automaticCheckTimer->isActive();
}

qint64 LicenseManager::millisecondsUntilNextAutomaticCheck() const
{
    if (!licensesEnabledByProfile() || !m_config.automaticLicenseChecks) {
        return -1;
    }

    const LicenseCheckResult persisted = readPersistedState();
    const QDateTime now = QDateTime::currentDateTimeUtc();

    const qint64 fallbackSeconds =
        static_cast<qint64>(qMax(5, m_config.licenseCheckIntervalMinutes)) * 60;

    const QDateTime serverDueAt = persisted.entitlement.nextOnlineCheckAfterUtc;
    if (serverDueAt.isValid() && serverDueAt > now) {
        return now.msecsTo(serverDueAt);
    }

    // A malformed or stale server timestamp must not create a rapid retry loop.
    // Fall back to a bounded interval measured from the last local check.
    QDateTime fallbackDueAt;
    if (persisted.checkedAtUtc.isValid()) {
        fallbackDueAt = persisted.checkedAtUtc.addSecs(fallbackSeconds);
    } else if (persisted.lastVerifiedAtUtc.isValid()) {
        fallbackDueAt = persisted.lastVerifiedAtUtc.addSecs(fallbackSeconds);
    }

    if (!fallbackDueAt.isValid() || fallbackDueAt <= now) {
        return 0;
    }
    return now.msecsTo(fallbackDueAt);
}

bool LicenseManager::shouldCheckAutomatically() const
{
    return licensesEnabledByProfile()
        && m_config.automaticLicenseChecks
        && millisecondsUntilNextAutomaticCheck() == 0;
}

QUrl LicenseManager::licenseStatusUrlFor(const AppConfig &config)
{
    QUrl url(config.licenseApiBaseUrl.trimmed());
    if (!isHttpUrl(url)) {
        return {};
    }

    QString path = url.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path.append(QLatin1Char('/'));
    }
    path.append(QStringLiteral("license-status.json"));
    url.setPath(path);
    return url;
}

QString LicenseManager::licenseStatePathFor(
    const AppConfig &config,
    const QString &appDataDirectoryOverride
)
{
    const QString directory = appDataDirectory(config, appDataDirectoryOverride);
    return directory.isEmpty()
        ? QString{}
        : QDir(directory).filePath(QString::fromLatin1(kStateFileName));
}

LicenseCheckResult LicenseManager::evaluateResponse(
    const AppConfig &config,
    const QByteArray &payload,
    const QString &endpointUrl
)
{
    LicenseCheckResult result;
    result.endpointUrl = AppLogger::sanitize(endpointUrl);
    result.checkedAtUtc = QDateTime::currentDateTimeUtc();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("License response is not valid JSON.");
        return result;
    }

    const QJsonObject envelope = document.object();
    if (envelope.value(QStringLiteral("schemaVersion")).toInt() != 1
        || !envelope.value(QStringLiteral("success")).toBool(false)
        || !envelope.value(QStringLiteral("data")).isObject()) {
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("License response envelope is invalid.");
        return result;
    }

    const QJsonObject data = envelope.value(QStringLiteral("data")).toObject();
    QString errorMessage;
    LicenseEntitlement entitlement;
    entitlement.schemaVersion = envelope.value(QStringLiteral("schemaVersion")).toInt();
    entitlement.licenseId = requiredString(data, QStringLiteral("licenseId"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.serial = requiredString(data, QStringLiteral("serial"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.productId = requiredString(data, QStringLiteral("productId"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.serverStatus = requiredString(data, QStringLiteral("status"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    if (!isServerStatus(entitlement.serverStatus)) {
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("License response contains an unknown status.");
        return result;
    }
    entitlement.planName = requiredString(data, QStringLiteral("planName"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.durationDays = requiredPositiveInteger(data, QStringLiteral("durationDays"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.remainingDays = requiredNonNegativeInteger(data, QStringLiteral("remainingDays"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.deviceLimit = requiredPositiveInteger(data, QStringLiteral("deviceLimit"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.activeDeviceCount = requiredNonNegativeInteger(data, QStringLiteral("activeDeviceCount"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    if (entitlement.activeDeviceCount > entitlement.deviceLimit) {
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("License response has an invalid device count.");
        return result;
    }
    entitlement.features = requiredStringArray(data, QStringLiteral("features"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.issuedAtUtc = requiredUtcDateTime(data, QStringLiteral("issuedAt"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.serverTimeUtc = requiredUtcDateTime(data, QStringLiteral("serverTime"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.expiresAtUtc = requiredUtcDateTime(data, QStringLiteral("expiresAt"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.graceUntilUtc = requiredUtcDateTime(data, QStringLiteral("graceUntil"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;
    entitlement.nextOnlineCheckAfterUtc = requiredUtcDateTime(data, QStringLiteral("nextOnlineCheckAfter"), &errorMessage);
    if (!errorMessage.isEmpty()) goto invalid;

    if (entitlement.productId != config.productId) {
        result.status = LicenseStatus::InvalidResponse;
        result.message = QStringLiteral("License response product ID does not match this application.");
        return result;
    }

    result.status = statusForServerValue(entitlement.serverStatus);
    result.entitlement = entitlement;
    result.lastVerifiedAtUtc = entitlement.serverTimeUtc;

    if (result.status == LicenseStatus::Active) {
        result.message = QStringLiteral("License is active. %1 day(s) remaining.").arg(entitlement.remainingDays);
    } else if (result.status == LicenseStatus::Grace) {
        result.message = QStringLiteral("License is in the server-provided grace period until %1.")
            .arg(utcText(entitlement.graceUntilUtc));
    } else if (result.status == LicenseStatus::Expired) {
        result.message = QStringLiteral("The license server reports that this license is expired.");
    } else {
        result.message = QStringLiteral("The license server reports that this license is revoked.");
    }
    return result;

invalid:
    result.status = LicenseStatus::InvalidResponse;
    result.message = errorMessage.isEmpty()
        ? QStringLiteral("License response is invalid.")
        : errorMessage;
    return result;
}

QString LicenseManager::statusText(LicenseStatus status)
{
    switch (status) {
    case LicenseStatus::NotChecked: return QObject::tr("Not checked yet");
    case LicenseStatus::Checking: return QObject::tr("Checking now");
    case LicenseStatus::Active: return QObject::tr("Active");
    case LicenseStatus::Grace: return QObject::tr("Grace period");
    case LicenseStatus::Expired: return QObject::tr("Expired");
    case LicenseStatus::Revoked: return QObject::tr("Revoked");
    case LicenseStatus::OfflineGrace: return QObject::tr("Offline grace");
    case LicenseStatus::VerificationRequired: return QObject::tr("Online verification required");
    case LicenseStatus::NetworkError: return QObject::tr("Network check failed");
    case LicenseStatus::InvalidResponse: return QObject::tr("Invalid server response");
    case LicenseStatus::Disabled: return QObject::tr("Disabled");
    }
    return QObject::tr("Unknown");
}

QString LicenseManager::redactedSerial(const QString &serial)
{
    const QString value = serial.trimmed();
    if (value.isEmpty() || value.contains(QChar(0x2022))) {
        return value;
    }

    const QString suffix = value.right(qMin(4, value.size()));
    return QStringLiteral("••••-••••-••••-%1").arg(suffix);
}

void LicenseManager::complete(LicenseCheckResult result)
{
    if (!result.checkedAtUtc.isValid()) {
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
    }

    m_lastResult = result;
    if (result.isTerminal() && result.status != LicenseStatus::Disabled) {
        persistState(result);
    }

    AppLogger::info(
        QStringLiteral("license"),
        QStringLiteral("License check completed. status=%1 endpoint=%2")
            .arg(statusText(result.status), result.endpointUrl)
    );
    emit checkCompleted(result);
    scheduleAutomaticCheck();
}

void LicenseManager::scheduleAutomaticCheck()
{
    m_automaticCheckTimer->stop();
    if (!m_automaticChecksStarted || !licensesEnabledByProfile() || !m_config.automaticLicenseChecks) {
        return;
    }

    const qint64 delay = millisecondsUntilNextAutomaticCheck();
    if (delay < 0) {
        return;
    }

    const qint64 minimumDelay = 250;
    const qint64 maximumDelay = static_cast<qint64>(std::numeric_limits<int>::max());
    const qint64 boundedDelay = qBound(minimumDelay, delay, maximumDelay);
    m_automaticCheckTimer->start(static_cast<int>(boundedDelay));
}

bool LicenseManager::licensesEnabledByProfile() const
{
    const QJsonObject features = m_config.effectiveConfig.value(QStringLiteral("features")).toObject();
    return features.value(QStringLiteral("licenseEnabled")).toBool(true);
}

LicenseCheckResult LicenseManager::offlineResultFor(const QString &networkMessage) const
{
    LicenseCheckResult cached = readPersistedState();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (!cached.entitlement.licenseId.isEmpty()) {
        cached.entitlement.nextOnlineCheckAfterUtc = now.addSecs(
            static_cast<qint64>(qMax(5, m_config.licenseCheckIntervalMinutes)) * 60
        );
    }

    if (cached.entitlement.serverStatus == QStringLiteral("revoked")) {
        cached.status = LicenseStatus::Revoked;
        cached.message = QStringLiteral("The last server-verified license state is revoked. %1").arg(networkMessage);
        return cached;
    }
    if (cached.entitlement.serverStatus == QStringLiteral("expired")) {
        cached.status = LicenseStatus::Expired;
        cached.message = QStringLiteral("The last server-verified license state is expired. %1").arg(networkMessage);
        return cached;
    }

    const bool usableCachedGrant = cached.entitlement.serverStatus == QStringLiteral("active")
        || cached.entitlement.serverStatus == QStringLiteral("grace");
    if (!usableCachedGrant || !cached.lastVerifiedAtUtc.isValid()) {
        LicenseCheckResult result;
        result.status = LicenseStatus::NetworkError;
        result.message = networkMessage;
        return result;
    }

    if (cached.entitlement.serverTimeUtc.isValid() && now < cached.entitlement.serverTimeUtc) {
        AppLogger::warning(
            QStringLiteral("license"),
            QStringLiteral("Local clock appears earlier than the last license server time; retaining cached status and retrying online verification.")
        );
    }

    if (cached.entitlement.graceUntilUtc.isValid() && now <= cached.entitlement.graceUntilUtc) {
        cached.status = LicenseStatus::OfflineGrace;
        cached.message = QStringLiteral("%1 Cached offline grace remains available until %2.")
            .arg(networkMessage, utcText(cached.entitlement.graceUntilUtc));
        return cached;
    }

    // Do not use the local clock alone to declare a license locked/expired.
    // The server remains authoritative; ask the user to reconnect instead.
    cached.status = LicenseStatus::VerificationRequired;
    cached.message = QStringLiteral("%1 Cached offline grace may have ended; online verification is required.")
        .arg(networkMessage);
    return cached;
}

void LicenseManager::persistState(const LicenseCheckResult &result) const
{
    const QString path = licenseStatePathFor(m_config, m_appDataDirectoryOverride);
    if (path.isEmpty()) {
        return;
    }

    const QJsonObject state{
        {QStringLiteral("schemaVersion"), kLicenseStateSchemaVersion},
        {QStringLiteral("status"), statusKey(result.status)},
        {QStringLiteral("message"), AppLogger::sanitize(result.message)},
        {QStringLiteral("endpointUrl"), AppLogger::sanitize(result.endpointUrl)},
        {QStringLiteral("checkedAt"), utcText(result.checkedAtUtc)},
        {QStringLiteral("lastVerifiedAt"), utcText(result.lastVerifiedAtUtc)},
        {QStringLiteral("entitlement"), entitlementToJson(result.entitlement)}
    };

    if (!writeJsonAtomically(path, state)) {
        AppLogger::warning(QStringLiteral("license"), QStringLiteral("Could not persist license state."));
    }
}

LicenseCheckResult LicenseManager::readPersistedState() const
{
    const QString path = licenseStatePathFor(m_config, m_appDataDirectoryOverride);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    const QJsonObject state = document.object();
    if (state.value(QStringLiteral("schemaVersion")).toInt() != kLicenseStateSchemaVersion) {
        return {};
    }

    LicenseCheckResult result;
    result.status = statusFromText(state.value(QStringLiteral("status")).toString());
    result.message = state.value(QStringLiteral("message")).toString();
    result.endpointUrl = state.value(QStringLiteral("endpointUrl")).toString();
    result.checkedAtUtc = parseUtc(state, QStringLiteral("checkedAt"));
    result.lastVerifiedAtUtc = parseUtc(state, QStringLiteral("lastVerifiedAt"));
    result.entitlement = entitlementFromJson(state.value(QStringLiteral("entitlement")).toObject());
    return result;
}
