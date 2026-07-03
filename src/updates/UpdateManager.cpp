#include "updates/UpdateManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <limits>
#include <utility>

#include "core/AppLogger.h"

namespace
{
constexpr int kUpdateStateSchemaVersion = 1;
constexpr auto kStateFileName = "update-state.json";

QString safeUrlText(const QUrl &url)
{
    return AppLogger::sanitize(url.toString(QUrl::FullyEncoded));
}

QString normalizedCpuArchitecture(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("x86_64") || normalized == QStringLiteral("amd64")
        || normalized == QStringLiteral("x64")) {
        return QStringLiteral("x64");
    }
    if (normalized == QStringLiteral("arm64") || normalized == QStringLiteral("aarch64")) {
        return QStringLiteral("arm64");
    }
    if (normalized == QStringLiteral("x86") || normalized == QStringLiteral("i386")
        || normalized == QStringLiteral("i686")) {
        return QStringLiteral("x86");
    }
    return normalized;
}

QString normalizedOs(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("osx") || normalized == QStringLiteral("macos")) {
        return QStringLiteral("macos");
    }
    if (normalized == QStringLiteral("win")) {
        return QStringLiteral("windows");
    }

    // Update artifacts target OS families, not individual Linux distributions.
    // QSysInfo::productType() can be "linuxmint", "ubuntu", "fedora", etc.,
    // while the CDN hierarchy uses the portable "linux" family segment.
    static const QStringList linuxDistributionNames{
        QStringLiteral("linux"),
        QStringLiteral("linuxmint"),
        QStringLiteral("ubuntu"),
        QStringLiteral("debian"),
        QStringLiteral("fedora"),
        QStringLiteral("rhel"),
        QStringLiteral("centos"),
        QStringLiteral("rocky"),
        QStringLiteral("almalinux"),
        QStringLiteral("arch"),
        QStringLiteral("manjaro"),
        QStringLiteral("opensuse"),
        QStringLiteral("suse"),
        QStringLiteral("alpine"),
        QStringLiteral("gentoo"),
        QStringLiteral("void"),
        QStringLiteral("nixos"),
        QStringLiteral("kali")
    };
    if (linuxDistributionNames.contains(normalized)) {
        return QStringLiteral("linux");
    }
    return normalized;
}

struct SemanticVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    QStringList prerelease;
};

bool parseNumericIdentifier(const QString &text, int *value)
{
    if (!QRegularExpression(QStringLiteral("^(0|[1-9]\\d*)$")).match(text).hasMatch()) {
        return false;
    }
    bool ok = false;
    const int parsed = text.toInt(&ok);
    if (!ok) {
        return false;
    }
    *value = parsed;
    return true;
}

bool parseSemver(const QString &text, SemanticVersion *version, QString *errorMessage)
{
    const QRegularExpression pattern(
        QStringLiteral("^v?((?:0|[1-9]\\d*))\\.((?:0|[1-9]\\d*))\\.((?:0|[1-9]\\d*))(?:-([0-9A-Za-z-]+(?:\\.[0-9A-Za-z-]+)*))?(?:\\+([0-9A-Za-z-]+(?:\\.[0-9A-Za-z-]+)*))?$")
    );
    const QRegularExpressionMatch match = pattern.match(text.trimmed());
    if (!match.hasMatch()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Version is not valid semantic versioning: %1").arg(text);
        }
        return false;
    }

    version->major = match.captured(1).toInt();
    version->minor = match.captured(2).toInt();
    version->patch = match.captured(3).toInt();
    version->prerelease = match.captured(4).isEmpty()
        ? QStringList{}
        : match.captured(4).split(QLatin1Char('.'));
    return true;
}

int comparePrerelease(const QStringList &left, const QStringList &right)
{
    if (left.isEmpty() && right.isEmpty()) return 0;
    if (left.isEmpty()) return 1;
    if (right.isEmpty()) return -1;

    const int count = qMin(left.size(), right.size());
    for (int index = 0; index < count; ++index) {
        int leftNumber = 0;
        int rightNumber = 0;
        const bool leftNumeric = parseNumericIdentifier(left.at(index), &leftNumber);
        const bool rightNumeric = parseNumericIdentifier(right.at(index), &rightNumber);
        if (leftNumeric && rightNumeric) {
            if (leftNumber != rightNumber) return leftNumber < rightNumber ? -1 : 1;
            continue;
        }
        if (leftNumeric != rightNumeric) return leftNumeric ? -1 : 1;
        if (left.at(index) != right.at(index)) return left.at(index) < right.at(index) ? -1 : 1;
    }
    if (left.size() == right.size()) return 0;
    return left.size() < right.size() ? -1 : 1;
}

bool isHttpUrl(const QString &value)
{
    const QUrl url(value);
    return url.isValid()
        && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))
        && !url.host().isEmpty();
}

QJsonObject objectValue(const QJsonDocument &document)
{
    return document.isObject() ? document.object() : QJsonObject{};
}

QString requiredString(const QJsonObject &object, const QString &key, QString *errorMessage)
{
    const QJsonValue value = object.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Manifest field '%1' must be a non-empty string.").arg(key);
        }
        return {};
    }
    return value.toString().trimmed();
}

bool validSha256(const QString &value)
{
    return QRegularExpression(QStringLiteral("^[A-Fa-f0-9]{64}$")).match(value).hasMatch();
}

QString appDataDirectory(const AppConfig &config, const QString &overrideDirectory)
{
    if (!overrideDirectory.isEmpty()) {
        return QDir::cleanPath(overrideDirectory);
    }
    return AppLogger::appDataDirectoryFor(config);
}

bool writeState(const QString &path, const UpdateCheckResult &result)
{
    if (path.isEmpty()) {
        return false;
    }
    const QFileInfo info(path);
    if (!QDir().mkpath(info.dir().absolutePath())) {
        return false;
    }

    QJsonObject object{
        {QStringLiteral("schemaVersion"), kUpdateStateSchemaVersion},
        {QStringLiteral("lastCheckedAt"), result.checkedAtUtc.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("status"), UpdateManager::statusText(result.status)},
        {QStringLiteral("currentVersion"), AppLogger::sanitize(result.currentVersion)},
        {QStringLiteral("availableVersion"), AppLogger::sanitize(result.availableVersion)},
        {QStringLiteral("manifestUrl"), AppLogger::sanitize(result.manifestUrl)},
        {QStringLiteral("message"), AppLogger::sanitize(result.message)}
    };

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

UpdateCheckResult stateResultFromJson(const QJsonObject &object)
{
    UpdateCheckResult result;
    const QString status = object.value(QStringLiteral("status")).toString();
    if (status == QStringLiteral("up-to-date")) {
        result.status = UpdateStatus::UpToDate;
    } else if (status == QStringLiteral("update-available")) {
        result.status = UpdateStatus::UpdateAvailable;
    } else if (status == QStringLiteral("network-error")) {
        result.status = UpdateStatus::NetworkError;
    } else if (status == QStringLiteral("invalid-manifest")) {
        result.status = UpdateStatus::InvalidManifest;
    } else if (status == QStringLiteral("unsupported-platform")) {
        result.status = UpdateStatus::UnsupportedPlatform;
    } else if (status == QStringLiteral("disabled")) {
        result.status = UpdateStatus::Disabled;
    }
    result.currentVersion = object.value(QStringLiteral("currentVersion")).toString();
    result.availableVersion = object.value(QStringLiteral("availableVersion")).toString();
    result.manifestUrl = object.value(QStringLiteral("manifestUrl")).toString();
    result.message = object.value(QStringLiteral("message")).toString();
    result.checkedAtUtc = QDateTime::fromString(object.value(QStringLiteral("lastCheckedAt")).toString(), Qt::ISODate);
    if (result.checkedAtUtc.isValid()) {
        result.checkedAtUtc = result.checkedAtUtc.toUTC();
    }
    return result;
}
} // namespace

bool UpdateCheckResult::isError() const
{
    return status == UpdateStatus::NetworkError
        || status == UpdateStatus::InvalidManifest
        || status == UpdateStatus::UnsupportedPlatform;
}

bool UpdateCheckResult::isTerminal() const
{
    return status != UpdateStatus::NotChecked && status != UpdateStatus::Checking;
}

UpdateManager::UpdateManager(
    const AppConfig &config,
    QObject *parent,
    QString platformOverride,
    QString architectureOverride,
    QString appDataDirectoryOverride
)
    : QObject(parent)
    , m_config(config)
    , m_network(new QNetworkAccessManager(this))
    , m_automaticCheckTimer(new QTimer(this))
    , m_platformOverride(std::move(platformOverride))
    , m_architectureOverride(std::move(architectureOverride))
    , m_appDataDirectoryOverride(std::move(appDataDirectoryOverride))
    , m_lastResult(readPersistedState())
{
    qRegisterMetaType<UpdateCheckResult>();
    m_automaticCheckTimer->setSingleShot(true);
    connect(m_automaticCheckTimer, &QTimer::timeout, this, [this] {
        if (!m_automaticChecksStarted) {
            return;
        }
        if (shouldCheckAutomatically()) {
            checkNow();
            return;
        }
        scheduleAutomaticCheck();
    });
}

void UpdateManager::setConfig(const AppConfig &config)
{
    m_config = config;
    if (m_automaticChecksStarted) {
        scheduleAutomaticCheck();
    }
}

const AppConfig &UpdateManager::config() const
{
    return m_config;
}

bool UpdateManager::isChecking() const
{
    return m_reply != nullptr;
}

UpdateCheckResult UpdateManager::lastResult() const
{
    return m_lastResult;
}

void UpdateManager::checkNow()
{
    if (m_reply != nullptr) {
        return;
    }

    if (!updatesEnabledByProfile()) {
        UpdateCheckResult disabled;
        disabled.status = UpdateStatus::Disabled;
        disabled.currentVersion = m_config.version;
        disabled.checkedAtUtc = QDateTime::currentDateTimeUtc();
        disabled.message = QStringLiteral("Update checks are disabled by the active product profile.");
        complete(disabled);
        return;
    }

    const QString platform = normalizedPlatform(m_platformOverride);
    const QString architecture = normalizedArchitecture(m_architectureOverride);
    if (platform.isEmpty() || architecture.isEmpty()) {
        UpdateCheckResult unsupported;
        unsupported.status = UpdateStatus::UnsupportedPlatform;
        unsupported.currentVersion = m_config.version;
        unsupported.checkedAtUtc = QDateTime::currentDateTimeUtc();
        unsupported.message = QStringLiteral("This platform or CPU architecture is not supported by the update channel.");
        complete(unsupported);
        return;
    }

    const QUrl manifestUrl = manifestUrlFor(m_config, platform, architecture);
    if (!manifestUrl.isValid() || manifestUrl.scheme().isEmpty()) {
        UpdateCheckResult invalid;
        invalid.status = UpdateStatus::InvalidManifest;
        invalid.currentVersion = m_config.version;
        invalid.checkedAtUtc = QDateTime::currentDateTimeUtc();
        invalid.message = QStringLiteral("The configured CDN URL cannot form a valid manifest URL.");
        complete(invalid);
        return;
    }

    m_lastResult = {};
    m_lastResult.status = UpdateStatus::Checking;
    m_lastResult.currentVersion = m_config.version;
    m_lastResult.manifestUrl = safeUrlText(manifestUrl);
    emit checkStarted();

    AppLogger::info(
        QStringLiteral("updates"),
        QStringLiteral("Checking update manifest: %1").arg(safeUrlText(manifestUrl))
    );

    QNetworkRequest request(manifestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MyAppTemplate/%1").arg(m_config.version));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_network->get(request);

    connect(m_reply, &QNetworkReply::finished, this, [this, platform, architecture] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        if (reply == nullptr) {
            return;
        }

        UpdateCheckResult result;
        result.currentVersion = m_config.version;
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
        result.manifestUrl = safeUrlText(reply->url());

        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            result.status = UpdateStatus::NetworkError;
            result.message = QStringLiteral("Could not fetch the update manifest%1: %2")
                .arg(httpStatus > 0 ? QStringLiteral(" (HTTP %1)").arg(httpStatus) : QString{})
                .arg(AppLogger::sanitize(reply->errorString()));
            reply->deleteLater();
            complete(result);
            return;
        }
        if (httpStatus >= 400) {
            result.status = UpdateStatus::NetworkError;
            result.message = QStringLiteral("The update server returned HTTP %1.").arg(httpStatus);
            reply->deleteLater();
            complete(result);
            return;
        }

        result = evaluateManifest(m_config, reply->readAll(), result.manifestUrl, platform, architecture);
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
        reply->deleteLater();
        complete(result);
    });
}

void UpdateManager::cancel()
{
    if (m_reply != nullptr) {
        m_reply->abort();
    }
}

void UpdateManager::startAutomaticChecks()
{
    m_automaticChecksStarted = true;
    scheduleAutomaticCheck();
}

void UpdateManager::stopAutomaticChecks()
{
    m_automaticChecksStarted = false;
    m_automaticCheckTimer->stop();
}

bool UpdateManager::automaticChecksRunning() const
{
    return m_automaticChecksStarted && m_automaticCheckTimer->isActive();
}

qint64 UpdateManager::millisecondsUntilNextAutomaticCheck() const
{
    if (!m_config.automaticUpdateChecks || !updatesEnabledByProfile()) {
        return -1;
    }
    if (!m_lastResult.checkedAtUtc.isValid()) {
        return 0;
    }

    const qint64 elapsedMilliseconds = m_lastResult.checkedAtUtc.msecsTo(QDateTime::currentDateTimeUtc());
    if (elapsedMilliseconds < 0) {
        // A clock moved backwards should never indefinitely suppress a check.
        return 0;
    }

    const qint64 intervalMilliseconds = qMax(1, m_config.updateCheckIntervalMinutes) * 60LL * 1000LL;
    return qMax<qint64>(0, intervalMilliseconds - elapsedMilliseconds);
}

bool UpdateManager::shouldCheckAutomatically() const
{
    return millisecondsUntilNextAutomaticCheck() == 0;
}

void UpdateManager::scheduleAutomaticCheck()
{
    m_automaticCheckTimer->stop();
    if (!m_automaticChecksStarted) {
        return;
    }

    const qint64 delay = millisecondsUntilNextAutomaticCheck();
    if (delay < 0) {
        return;
    }

    const int boundedDelay = static_cast<int>(qMin<qint64>(delay, std::numeric_limits<int>::max()));
    m_automaticCheckTimer->start(boundedDelay);
}

bool UpdateManager::updatesEnabledByProfile() const
{
    if (m_config.effectiveConfig.isEmpty()) {
        return true;
    }
    const QJsonObject features = m_config.effectiveConfig.value(QStringLiteral("features")).toObject();
    return features.value(QStringLiteral("updatesEnabled")).toBool(true);
}

QString UpdateManager::normalizedPlatform(const QString &value)
{
    return normalizedOs(value.isEmpty() ? QSysInfo::productType() : value);
}

QString UpdateManager::normalizedArchitecture(const QString &value)
{
    return normalizedCpuArchitecture(value.isEmpty() ? QSysInfo::currentCpuArchitecture() : value);
}

QUrl UpdateManager::manifestUrlFor(
    const AppConfig &config,
    const QString &platform,
    const QString &architecture
)
{
    QUrl url(config.cdnBaseUrl);
    if (!url.isValid() || url.scheme().isEmpty()) {
        return {};
    }

    QString path = url.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path.append(QLatin1Char('/'));
    }
    path.append(QStringLiteral("%1/%2/%3/manifest.json")
                    .arg(config.updateChannel, normalizedPlatform(platform), normalizedArchitecture(architecture)));
    url.setPath(path);
    url.setQuery(QUrlQuery{});
    return url;
}

QString UpdateManager::updateStatePathFor(
    const AppConfig &config,
    const QString &appDataDirectoryOverride
)
{
    const QString directory = appDataDirectory(config, appDataDirectoryOverride);
    return directory.isEmpty()
        ? QString{}
        : QDir(directory).filePath(QString::fromLatin1(kStateFileName));
}

int UpdateManager::compareSemanticVersions(
    const QString &left,
    const QString &right,
    QString *errorMessage
)
{
    SemanticVersion leftVersion;
    SemanticVersion rightVersion;
    QString localError;
    if (!parseSemver(left, &leftVersion, &localError) || !parseSemver(right, &rightVersion, &localError)) {
        if (errorMessage != nullptr) {
            *errorMessage = localError;
        }
        return 0;
    }

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    if (leftVersion.major != rightVersion.major) return leftVersion.major < rightVersion.major ? -1 : 1;
    if (leftVersion.minor != rightVersion.minor) return leftVersion.minor < rightVersion.minor ? -1 : 1;
    if (leftVersion.patch != rightVersion.patch) return leftVersion.patch < rightVersion.patch ? -1 : 1;
    return comparePrerelease(leftVersion.prerelease, rightVersion.prerelease);
}

UpdateCheckResult UpdateManager::evaluateManifest(
    const AppConfig &config,
    const QByteArray &payload,
    const QString &manifestUrl,
    const QString &platform,
    const QString &architecture
)
{
    UpdateCheckResult result;
    result.currentVersion = config.version;
    result.manifestUrl = AppLogger::sanitize(manifestUrl);

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.status = UpdateStatus::InvalidManifest;
        result.message = QStringLiteral("The update manifest is not valid JSON.");
        return result;
    }

    const QJsonObject object = objectValue(document);
    QString error;
    const int schemaVersion = object.value(QStringLiteral("schemaVersion")).toInt();
    if (schemaVersion != 1) {
        result.status = UpdateStatus::InvalidManifest;
        result.message = QStringLiteral("Unsupported update manifest schema version.");
        return result;
    }

    UpdateManifest manifest;
    manifest.schemaVersion = schemaVersion;
    manifest.productId = requiredString(object, QStringLiteral("productId"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.channel = requiredString(object, QStringLiteral("channel"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.platform = requiredString(object, QStringLiteral("platform"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.architecture = requiredString(object, QStringLiteral("architecture"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.version = requiredString(object, QStringLiteral("version"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.artifactFileName = requiredString(object, QStringLiteral("artifactFileName"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.artifactUrl = requiredString(object, QStringLiteral("artifactUrl"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.sha256 = requiredString(object, QStringLiteral("sha256"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.releaseNotesUrl = requiredString(object, QStringLiteral("releaseNotesUrl"), &error);
    if (!error.isEmpty()) goto invalid;
    manifest.sizeBytes = static_cast<qint64>(object.value(QStringLiteral("sizeBytes")).toDouble(-1));
    manifest.mandatory = object.value(QStringLiteral("mandatory")).toBool(false);
    manifest.publishedAt = QDateTime::fromString(object.value(QStringLiteral("publishedAt")).toString(), Qt::ISODate);

    if (manifest.productId != config.productId) {
        error = QStringLiteral("Manifest product ID does not match this application.");
        goto invalid;
    }
    if (manifest.channel != config.updateChannel) {
        error = QStringLiteral("Manifest channel does not match the active update channel.");
        goto invalid;
    }
    if (normalizedOs(manifest.platform) != normalizedPlatform(platform)
        || normalizedCpuArchitecture(manifest.architecture) != normalizedArchitecture(architecture)) {
        result.status = UpdateStatus::UnsupportedPlatform;
        result.message = QStringLiteral("No compatible release is published for this platform and CPU architecture.");
        result.manifest = manifest;
        return result;
    }
    if (!manifest.publishedAt.isValid()) {
        error = QStringLiteral("Manifest field 'publishedAt' must be an ISO-8601 date/time.");
        goto invalid;
    }
    if (manifest.sizeBytes < 0) {
        error = QStringLiteral("Manifest field 'sizeBytes' must be zero or greater.");
        goto invalid;
    }
    if (!validSha256(manifest.sha256)) {
        error = QStringLiteral("Manifest field 'sha256' must be a 64-character hexadecimal digest.");
        goto invalid;
    }
    if (!isHttpUrl(manifest.artifactUrl) || !isHttpUrl(manifest.releaseNotesUrl)) {
        error = QStringLiteral("Manifest artifact and release-notes URLs must be valid URLs.");
        goto invalid;
    }
    {
        QString versionError;
        const int comparison = compareSemanticVersions(config.version, manifest.version, &versionError);
        if (!versionError.isEmpty()) {
            error = versionError;
            goto invalid;
        }
        result.manifest = manifest;
        result.availableVersion = manifest.version;
        if (comparison < 0) {
            result.status = UpdateStatus::UpdateAvailable;
            result.message = QStringLiteral("Version %1 is available.").arg(manifest.version);
        } else {
            result.status = UpdateStatus::UpToDate;
            result.message = comparison == 0
                ? QStringLiteral("This application is up to date.")
                : QStringLiteral("The current application version is newer than the published channel manifest.");
        }
        return result;
    }

invalid:
    result.status = UpdateStatus::InvalidManifest;
    result.message = error.isEmpty()
        ? QStringLiteral("The update manifest is invalid.")
        : error;
    return result;
}

QString UpdateManager::statusText(UpdateStatus status)
{
    switch (status) {
    case UpdateStatus::NotChecked: return QStringLiteral("not-checked");
    case UpdateStatus::Checking: return QStringLiteral("checking");
    case UpdateStatus::UpToDate: return QStringLiteral("up-to-date");
    case UpdateStatus::UpdateAvailable: return QStringLiteral("update-available");
    case UpdateStatus::NetworkError: return QStringLiteral("network-error");
    case UpdateStatus::InvalidManifest: return QStringLiteral("invalid-manifest");
    case UpdateStatus::UnsupportedPlatform: return QStringLiteral("unsupported-platform");
    case UpdateStatus::Disabled: return QStringLiteral("disabled");
    }
    return QStringLiteral("unknown");
}

void UpdateManager::complete(UpdateCheckResult result)
{
    if (!result.checkedAtUtc.isValid()) {
        result.checkedAtUtc = QDateTime::currentDateTimeUtc();
    }
    result.manifestUrl = AppLogger::sanitize(result.manifestUrl);
    result.message = AppLogger::sanitize(result.message);
    m_lastResult = result;
    persistState(result);

    const auto log = result.isError() ? AppLogger::warning : AppLogger::info;
    log(
        QStringLiteral("updates"),
        QStringLiteral("Update check finished. status=%1 current=%2 available=%3 message=%4")
            .arg(statusText(result.status), result.currentVersion, result.availableVersion, result.message)
    );
    emit checkCompleted(result);
    scheduleAutomaticCheck();
}

void UpdateManager::persistState(const UpdateCheckResult &result) const
{
    const QString path = updateStatePathFor(m_config, m_appDataDirectoryOverride);
    if (!writeState(path, result)) {
        AppLogger::warning(QStringLiteral("updates"), QStringLiteral("Could not persist update check state."));
    }
}

UpdateCheckResult UpdateManager::readPersistedState() const
{
    const QString path = updateStatePathFor(m_config, m_appDataDirectoryOverride);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }
    return stateResultFromJson(document.object());
}
