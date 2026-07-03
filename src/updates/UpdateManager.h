#pragma once

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QUrl>
#include <QMetaType>

#include "core/ConfigManager.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

/** Outcome of a manifest-only update check. No artifact is downloaded here. */
enum class UpdateStatus {
    NotChecked,
    Checking,
    UpToDate,
    UpdateAvailable,
    NetworkError,
    InvalidManifest,
    UnsupportedPlatform,
    Disabled
};

struct UpdateManifest
{
    int schemaVersion = 0;
    QString productId;
    QString channel;
    QString platform;
    QString architecture;
    QString version;
    QDateTime publishedAt;
    QString artifactFileName;
    QString artifactUrl;
    QString sha256;
    qint64 sizeBytes = 0;
    QString releaseNotesUrl;
    bool mandatory = false;
};

struct UpdateCheckResult
{
    UpdateStatus status = UpdateStatus::NotChecked;
    QString message;
    QString currentVersion;
    QString availableVersion;
    QString manifestUrl;
    QDateTime checkedAtUtc;
    UpdateManifest manifest;

    [[nodiscard]] bool isError() const;
    [[nodiscard]] bool isTerminal() const;
};

/**
 * Manifest-only update subsystem.
 *
 * v0.1.4 deliberately stops after checking a signed-release style manifest.
 * It does not download, install, restart, or replace the running executable.
 */
class UpdateManager final : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(
        const AppConfig &config,
        QObject *parent = nullptr,
        QString platformOverride = {},
        QString architectureOverride = {},
        QString appDataDirectoryOverride = {}
    );

    void setConfig(const AppConfig &config);
    [[nodiscard]] const AppConfig &config() const;
    [[nodiscard]] bool isChecking() const;
    [[nodiscard]] UpdateCheckResult lastResult() const;

    void checkNow();
    void cancel();

    /** Starts recurring automatic checks using the persisted last-check time. */
    void startAutomaticChecks();
    void stopAutomaticChecks();
    [[nodiscard]] bool automaticChecksRunning() const;
    /** Returns -1 when disabled, otherwise the delay until the next automatic check. */
    [[nodiscard]] qint64 millisecondsUntilNextAutomaticCheck() const;
    [[nodiscard]] bool shouldCheckAutomatically() const;

    [[nodiscard]] static QString normalizedPlatform(const QString &value = {});
    [[nodiscard]] static QString normalizedArchitecture(const QString &value = {});
    [[nodiscard]] static QUrl manifestUrlFor(
        const AppConfig &config,
        const QString &platform = {},
        const QString &architecture = {}
    );
    [[nodiscard]] static QString updateStatePathFor(
        const AppConfig &config,
        const QString &appDataDirectoryOverride = {}
    );
    [[nodiscard]] static int compareSemanticVersions(
        const QString &left,
        const QString &right,
        QString *errorMessage = nullptr
    );
    [[nodiscard]] static UpdateCheckResult evaluateManifest(
        const AppConfig &config,
        const QByteArray &payload,
        const QString &manifestUrl,
        const QString &platform = {},
        const QString &architecture = {}
    );
    [[nodiscard]] static QString statusText(UpdateStatus status);

signals:
    void checkStarted();
    void checkCompleted(const UpdateCheckResult &result);

private:
    void complete(UpdateCheckResult result);
    void scheduleAutomaticCheck();
    [[nodiscard]] bool updatesEnabledByProfile() const;
    void persistState(const UpdateCheckResult &result) const;
    [[nodiscard]] UpdateCheckResult readPersistedState() const;

    AppConfig m_config;
    QNetworkAccessManager *m_network = nullptr;
    QTimer *m_automaticCheckTimer = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_platformOverride;
    QString m_architectureOverride;
    QString m_appDataDirectoryOverride;
    UpdateCheckResult m_lastResult;
    bool m_automaticChecksStarted = false;
};

Q_DECLARE_METATYPE(UpdateCheckResult)
