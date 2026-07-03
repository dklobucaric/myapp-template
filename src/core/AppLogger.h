#pragma once

#include <QString>

#include "core/ConfigManager.h"

/**
 * Small, dependency-free application logger.
 *
 * Log messages are written locally only. The logger deliberately redacts common
 * secret-shaped values and private runtime paths before any data reaches disk.
 */
class AppLogger final
{
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    /**
     * Configures local logging. When baseDirectoryOverride is empty, the
     * directory containing the runtime config.json is used as the AppData root.
     */
    static void configure(const AppConfig &config, const QString &baseDirectoryOverride = {});

    [[nodiscard]] static QString appDataDirectoryFor(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );
    [[nodiscard]] static QString logDirectoryFor(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );
    [[nodiscard]] static QString logFilePathFor(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );

    [[nodiscard]] static QString logDirectory();
    [[nodiscard]] static QString logFilePath();
    [[nodiscard]] static bool isEnabled(Level level);

    static void debug(const QString &category, const QString &message);
    static void info(const QString &category, const QString &message);
    static void warning(const QString &category, const QString &message);
    static void error(const QString &category, const QString &message);

    /** Returns a version of text that is safe to write to local diagnostics. */
    [[nodiscard]] static QString sanitize(const QString &text);

    // Test-only helpers. They do not affect public runtime configuration.
    static void resetForTests();
    static void setRotationLimitsForTests(qint64 maxBytes, int archiveCount);

private:
    static void write(Level level, const QString &category, const QString &message);
};
