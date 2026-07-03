#pragma once

#include <QString>

#include "core/ConfigManager.h"

struct DiagnosticsResult
{
    bool success = false;
    QString packagePath;
    QString errorMessage;
};

/**
 * Creates a local, safe support ZIP. The archive is never uploaded by the app.
 * It contains a runtime summary, a redacted config summary and a bounded,
 * redacted excerpt of the current log file.
 */
class DiagnosticsManager final
{
public:
    [[nodiscard]] static QString diagnosticsDirectoryFor(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );

    [[nodiscard]] static DiagnosticsResult createSafeSupportPackage(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );

    [[nodiscard]] static QString safeConfigSummary(const AppConfig &config);
    [[nodiscard]] static QString safeLogExcerpt(
        const AppConfig &config,
        const QString &baseDirectoryOverride = {}
    );
};
