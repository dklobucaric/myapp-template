#include "core/ConfigManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
constexpr auto kAppProfileResource = ":/defaults/app-profile.json";
constexpr auto kDefaultConfigResource = ":/defaults/config.json";
constexpr auto kProfileResourcePrefix = ":/profiles/";
constexpr auto kUserConfigFileName = "config.json";

QString stringValue(
    const QJsonObject &object,
    const QString &key,
    const QString &fallback = {}
)
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

int intValue(const QJsonObject &object, const QString &key, int fallback)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

bool boolValue(const QJsonObject &object, const QString &key, bool fallback)
{
    const QJsonValue value = object.value(key);
    return value.isBool() ? value.toBool() : fallback;
}

double doubleValue(const QJsonObject &object, const QString &key, double fallback)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble(fallback) : fallback;
}

QJsonObject objectValue(const QJsonObject &object, const QString &key)
{
    return object.value(key).toObject();
}

QString normalizedEnvironment(const QString &environment)
{
    const QString normalized = environment.trimmed().toLower();

    if (normalized == "development" || normalized == "staging" || normalized == "production") {
        return normalized;
    }

    return {};
}
} // namespace

bool ConfigLoadResult::isUsable() const
{
    return !config.appName.isEmpty() && !config.productId.isEmpty();
}

ConfigLoadResult ConfigManager::load(
    const QString &requestedEnvironment,
    const QString &configDirectoryOverride
)
{
    ConfigLoadResult result;

    QString errorMessage;
    const QJsonObject appProfile = readJsonObject(kAppProfileResource, &errorMessage);
    if (!errorMessage.isEmpty()) {
        result.warnings.append(
            QStringLiteral("Could not load built-in app profile: %1").arg(errorMessage)
        );
    }

    errorMessage.clear();
    const QJsonObject defaultConfig = readJsonObject(kDefaultConfigResource, &errorMessage);
    if (!errorMessage.isEmpty()) {
        result.warnings.append(
            QStringLiteral("Could not load built-in default config: %1").arg(errorMessage)
        );
    }

    const QString requestedEnvironmentValue = requestedEnvironment.trimmed();
    QString environment = normalizedEnvironment(requestedEnvironmentValue);

    if (!requestedEnvironmentValue.isEmpty() && environment.isEmpty()) {
        result.warnings.append(
            QStringLiteral("Unknown environment '%1'; falling back to the app profile.")
                .arg(requestedEnvironmentValue)
        );
    }

    if (environment.isEmpty()) {
        environment = normalizedEnvironment(stringValue(appProfile, "environment", "development"));
    }
    if (environment.isEmpty()) {
        environment = QStringLiteral("development");
        result.warnings.append(
            QStringLiteral("App profile environment is invalid; falling back to development.")
        );
    }

    const QString profileResourcePath =
        QString::fromLatin1(kProfileResourcePrefix) + environment + QStringLiteral(".json");

    errorMessage.clear();
    const QJsonObject environmentProfile = readJsonObject(profileResourcePath, &errorMessage);
    if (!errorMessage.isEmpty()) {
        result.warnings.append(
            QStringLiteral("Could not load %1 profile: %2")
                .arg(environment, errorMessage)
        );
    }

    QJsonObject effectiveConfig = mergeObjects(
        defaultConfig,
        appProfileDefaultsAsConfig(appProfile)
    );

    QJsonObject profileOverlay = environmentProfile;
    profileOverlay.remove(QStringLiteral("environment"));
    effectiveConfig = mergeObjects(effectiveConfig, profileOverlay);

    const QString configPath = userConfigPath(configDirectoryOverride);
    const QFileInfo configFileInfo(configPath);
    const QDir configDirectory = configFileInfo.dir();

    if (!configDirectory.exists() && !QDir().mkpath(configDirectory.absolutePath())) {
        result.warnings.append(
            QStringLiteral("Could not create config directory: %1")
                .arg(configDirectory.absolutePath())
        );
    }

    QJsonObject userOverrides;
    if (!QFile::exists(configPath)) {
        const QJsonObject initialUserConfig{
            {QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("appearance"), QJsonObject{}},
            {QStringLiteral("layout"), QJsonObject{}},
            {QStringLiteral("updates"), QJsonObject{}},
            {QStringLiteral("services"), QJsonObject{}},
            {QStringLiteral("diagnostics"), QJsonObject{}}
        };

        errorMessage.clear();
        if (writeJsonAtomically(configPath, initialUserConfig, &errorMessage)) {
            result.config.userConfigCreated = true;
        } else {
            result.warnings.append(
                QStringLiteral("Could not create user config: %1").arg(errorMessage)
            );
        }
    } else {
        errorMessage.clear();
        userOverrides = readJsonObject(configPath, &errorMessage);
        if (!errorMessage.isEmpty()) {
            result.warnings.append(
                QStringLiteral("User config could not be read and was ignored: %1")
                    .arg(errorMessage)
            );
        }
    }

    effectiveConfig = mergeObjects(effectiveConfig, userOverrides);

    const QJsonObject appearance = objectValue(effectiveConfig, QStringLiteral("appearance"));
    const QJsonObject layout = objectValue(effectiveConfig, QStringLiteral("layout"));
    const QJsonObject updates = objectValue(effectiveConfig, QStringLiteral("updates"));
    const QJsonObject services = objectValue(effectiveConfig, QStringLiteral("services"));
    const QJsonObject diagnostics = objectValue(effectiveConfig, QStringLiteral("diagnostics"));
    const QJsonObject appDefaults = objectValue(appProfile, QStringLiteral("defaults"));

    AppConfig &config = result.config;
    config.appName = stringValue(appProfile, QStringLiteral("appName"), QStringLiteral("MyAppTemplate"));
    config.appId = stringValue(appProfile, QStringLiteral("appId"));
    config.productId = stringValue(appProfile, QStringLiteral("productId"));
    config.vendor = stringValue(appProfile, QStringLiteral("vendor"));
    config.version = stringValue(appProfile, QStringLiteral("version"));
    config.environment = environment;

    config.theme = stringValue(appearance, QStringLiteral("theme"), QStringLiteral("system"));
    config.accentColor = stringValue(appearance, QStringLiteral("accentColor"), QStringLiteral("#3B82F6"));
    config.fontScale = doubleValue(appearance, QStringLiteral("fontScale"), 1.0);
    config.density = stringValue(appearance, QStringLiteral("density"), QStringLiteral("comfortable"));

    config.windowWidth = intValue(
        layout,
        QStringLiteral("windowWidth"),
        intValue(appDefaults, QStringLiteral("windowWidth"), 1200)
    );
    config.windowHeight = intValue(
        layout,
        QStringLiteral("windowHeight"),
        intValue(appDefaults, QStringLiteral("windowHeight"), 800)
    );
    config.rememberWindowSize = boolValue(layout, QStringLiteral("rememberWindowSize"), true);
    config.rememberWindowPosition = boolValue(layout, QStringLiteral("rememberWindowPosition"), true);
    config.showStatusBar = boolValue(
        layout,
        QStringLiteral("showStatusBar"),
        boolValue(appDefaults, QStringLiteral("showStatusBar"), true)
    );
    config.showToolbar = boolValue(layout, QStringLiteral("showToolbar"), true);

    config.updateChannel = stringValue(updates, QStringLiteral("channel"), QStringLiteral("development"));
    config.automaticUpdateChecks = boolValue(updates, QStringLiteral("automaticChecks"), true);
    config.updateCheckIntervalMinutes = intValue(
        updates,
        QStringLiteral("checkIntervalMinutes"),
        intValue(appDefaults, QStringLiteral("updateCheckIntervalMinutes"), 60)
    );
    config.autoDownloadUpdates = boolValue(updates, QStringLiteral("autoDownload"), false);
    config.licenseCheckIntervalMinutes = intValue(
        appDefaults,
        QStringLiteral("licenseCheckIntervalMinutes"),
        60
    );

    config.cdnBaseUrl = stringValue(services, QStringLiteral("cdnBaseUrl"));
    config.licensePortalUrl = stringValue(services, QStringLiteral("licensePortalUrl"));
    config.licenseApiBaseUrl = stringValue(services, QStringLiteral("licenseApiBaseUrl"));

    config.loggingEnabled = boolValue(diagnostics, QStringLiteral("loggingEnabled"), true);
    config.loggingLevel = stringValue(
        diagnostics,
        QStringLiteral("loggingLevel"),
        stringValue(appDefaults, QStringLiteral("loggingLevel"), QStringLiteral("info"))
    );

    config.userConfigPath = configPath;
    config.effectiveConfig = effectiveConfig;

    return result;
}

QString ConfigManager::userConfigPath(const QString &configDirectoryOverride)
{
    QString configDirectory = configDirectoryOverride;

    if (configDirectory.isEmpty()) {
        configDirectory = QStandardPaths::writableLocation(
            QStandardPaths::AppLocalDataLocation
        );
    }

    return QDir(configDirectory).filePath(kUserConfigFileName);
}

bool ConfigManager::saveUserOverrides(
    const QJsonObject &overrides,
    const QString &configDirectoryOverride,
    QString *errorMessage
)
{
    return writeJsonAtomically(
        userConfigPath(configDirectoryOverride),
        overrides,
        errorMessage
    );
}

QJsonObject ConfigManager::readJsonObject(
    const QString &path,
    QString *errorMessage
)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString());
        }
        return {};
    }

    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Expected a JSON object.");
        }
        return {};
    }

    return document.object();
}

QJsonObject ConfigManager::mergeObjects(
    const QJsonObject &base,
    const QJsonObject &overlay
)
{
    QJsonObject merged = base;

    for (auto iterator = overlay.begin(); iterator != overlay.end(); ++iterator) {
        const QJsonValue existingValue = merged.value(iterator.key());
        const QJsonValue overlayValue = iterator.value();

        if (existingValue.isObject() && overlayValue.isObject()) {
            merged.insert(
                iterator.key(),
                mergeObjects(existingValue.toObject(), overlayValue.toObject())
            );
            continue;
        }

        merged.insert(iterator.key(), overlayValue);
    }

    return merged;
}

QJsonObject ConfigManager::appProfileDefaultsAsConfig(
    const QJsonObject &appProfile
)
{
    const QJsonObject defaults = objectValue(appProfile, QStringLiteral("defaults"));

    QJsonObject config;
    config.insert(
        QStringLiteral("appearance"),
        QJsonObject{
            {QStringLiteral("theme"), stringValue(defaults, QStringLiteral("theme"), QStringLiteral("system"))},
            {QStringLiteral("accentColor"), stringValue(defaults, QStringLiteral("accentColor"), QStringLiteral("#3B82F6"))}
        }
    );
    config.insert(
        QStringLiteral("layout"),
        QJsonObject{
            {QStringLiteral("windowWidth"), intValue(defaults, QStringLiteral("windowWidth"), 1200)},
            {QStringLiteral("windowHeight"), intValue(defaults, QStringLiteral("windowHeight"), 800)},
            {QStringLiteral("showStatusBar"), boolValue(defaults, QStringLiteral("showStatusBar"), true)}
        }
    );
    config.insert(
        QStringLiteral("updates"),
        QJsonObject{
            {QStringLiteral("checkIntervalMinutes"), intValue(defaults, QStringLiteral("updateCheckIntervalMinutes"), 60)}
        }
    );
    config.insert(
        QStringLiteral("diagnostics"),
        QJsonObject{
            {QStringLiteral("loggingLevel"), stringValue(defaults, QStringLiteral("loggingLevel"), QStringLiteral("info"))}
        }
    );

    return config;
}

bool ConfigManager::writeJsonAtomically(
    const QString &path,
    const QJsonObject &object,
    QString *errorMessage
)
{
    const QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.dir().absolutePath())) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not create directory: %1")
                .arg(fileInfo.dir().absolutePath());
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    return true;
}
