#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

/**
 * Runtime configuration assembled from the application profile, an optional
 * environment profile and the user's local config.json overrides.
 */
struct AppConfig
{
    QString appName;
    QString appId;
    QString productId;
    QString vendor;
    QString version;
    QString environment;

    QString theme;
    QString accentColor;
    double fontScale = 1.0;
    QString density;

    int windowWidth = 1200;
    int windowHeight = 800;
    bool rememberWindowSize = true;
    bool rememberWindowPosition = true;
    bool showStatusBar = true;
    bool showToolbar = true;

    QString updateChannel;
    bool automaticUpdateChecks = true;
    int updateCheckIntervalMinutes = 60;
    bool autoDownloadUpdates = false;
    int licenseCheckIntervalMinutes = 60;

    QString cdnBaseUrl;
    QString licensePortalUrl;
    QString licenseApiBaseUrl;

    bool loggingEnabled = true;
    QString loggingLevel;

    QString userConfigPath;
    bool userConfigCreated = false;

    QJsonObject effectiveConfig;
};

struct ConfigLoadResult
{
    AppConfig config;
    QStringList warnings;

    [[nodiscard]] bool isUsable() const;
};

/**
 * Loads the reusable configuration layers for MyAppTemplate.
 *
 * Load order, from lowest to highest priority:
 * 1. Built-in default config
 * 2. Built-in app profile defaults
 * 3. Built-in environment profile
 * 4. User config.json overrides
 */
class ConfigManager final
{
public:
    static ConfigLoadResult load(
        const QString &requestedEnvironment = {},
        const QString &configDirectoryOverride = {}
    );

    [[nodiscard]] static QString userConfigPath(
        const QString &configDirectoryOverride = {}
    );

    static bool saveUserOverrides(
        const QJsonObject &overrides,
        const QString &configDirectoryOverride,
        QString *errorMessage = nullptr
    );

private:
    static QJsonObject readJsonObject(
        const QString &path,
        QString *errorMessage = nullptr
    );

    static QJsonObject mergeObjects(
        const QJsonObject &base,
        const QJsonObject &overlay
    );

    static QJsonObject appProfileDefaultsAsConfig(
        const QJsonObject &appProfile
    );

    static bool writeJsonAtomically(
        const QString &path,
        const QJsonObject &object,
        QString *errorMessage = nullptr
    );
};
