#include "workspace/TemplateDashboard.h"

#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QSysInfo>
#include <QVBoxLayout>

#include "core/AppLogger.h"

namespace
{
QString updateStatusLabel(const UpdateCheckResult &result)
{
    switch (result.status) {
    case UpdateStatus::NotChecked: return QObject::tr("Not checked yet");
    case UpdateStatus::Checking: return QObject::tr("Checking now");
    case UpdateStatus::UpToDate: return QObject::tr("Up to date");
    case UpdateStatus::UpdateAvailable: return QObject::tr("Update available");
    case UpdateStatus::NetworkError: return QObject::tr("Network check failed");
    case UpdateStatus::InvalidManifest: return QObject::tr("Manifest invalid");
    case UpdateStatus::UnsupportedPlatform: return QObject::tr("Platform unsupported");
    case UpdateStatus::Disabled: return QObject::tr("Disabled");
    }
    return QObject::tr("Unknown");
}

QString licenseStatusLabel(const LicenseCheckResult &result)
{
    return LicenseManager::statusText(result.status);
}

}

TemplateDashboard::TemplateDashboard(
    const AppConfig &config,
    const UpdateCheckResult &updateResult,
    const LicenseCheckResult &licenseResult,
    const QStringList &warnings,
    QWidget *parent
)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *titleLabel = new QLabel(config.appName, this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 10);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *subtitleLabel = new QLabel(
        tr("Reusable Desktop Application Template · %1 profile").arg(config.environment), this
    );
    subtitleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);

    if (!warnings.isEmpty()) {
        layout->addWidget(createSection(tr("Configuration Warnings"), warnings));
    }

    layout->addWidget(createSection(
        tr("Application"),
        {tr("Name: %1").arg(config.appName), tr("Product ID: %1").arg(config.productId),
         tr("Version: %1").arg(config.version), tr("Platform: %1").arg(QSysInfo::prettyProductName()),
         tr("Environment: %1").arg(config.environment)}
    ));

    layout->addWidget(createSection(
        tr("Configuration"),
        {config.userConfigCreated ? tr("Status: Created initial user config") : tr("Status: Loaded user config"),
         tr("User config: %1").arg(config.userConfigPath),
         tr("Theme: %1 · Accent: %2").arg(config.theme, config.accentColor),
         tr("Window: %1 × %2").arg(config.windowWidth).arg(config.windowHeight)}
    ));

    QStringList licenseLines{
        tr("Status: %1").arg(licenseStatusLabel(licenseResult)),
        tr("License portal: %1").arg(config.licensePortalUrl),
        tr("License API: %1").arg(config.licenseApiBaseUrl),
        tr("Automatic checks: every %1 minutes").arg(config.licenseCheckIntervalMinutes)
    };
    if (!licenseResult.entitlement.licenseId.isEmpty()) {
        licenseLines.append(
            tr("Serial: %1").arg(LicenseManager::redactedSerial(licenseResult.entitlement.serial))
        );
        licenseLines.append(tr("Plan: %1").arg(licenseResult.entitlement.planName));
        licenseLines.append(
            tr("Duration: %1 days").arg(licenseResult.entitlement.durationDays)
        );
        licenseLines.append(
            tr("Devices: %1 / %2")
                .arg(licenseResult.entitlement.activeDeviceCount)
                .arg(licenseResult.entitlement.deviceLimit)
        );
    }
    if (licenseResult.entitlement.expiresAtUtc.isValid()) {
        licenseLines.append(
            tr("Expires (server): %1")
                .arg(licenseResult.entitlement.expiresAtUtc.toUTC().toString(Qt::ISODate))
        );
    }
    if (licenseResult.lastVerifiedAtUtc.isValid()) {
        licenseLines.append(
            tr("Last verified (server): %1")
                .arg(licenseResult.lastVerifiedAtUtc.toUTC().toString(Qt::ISODate))
        );
    }
    if (!licenseResult.message.isEmpty()) {
        licenseLines.append(tr("Result: %1").arg(licenseResult.message));
    }
    licenseLines.append(tr("The license server controls seats, duration, expiry and revocation."));
    layout->addWidget(createSection(tr("License"), licenseLines));

    QStringList updateLines{
        tr("Channel: %1").arg(config.updateChannel),
        tr("CDN: %1").arg(config.cdnBaseUrl),
        tr("Automatic checks: every %1 minutes").arg(config.updateCheckIntervalMinutes),
        tr("Status: %1").arg(updateStatusLabel(updateResult))
    };
    if (!updateResult.availableVersion.isEmpty()) {
        updateLines.append(tr("Available version: %1").arg(updateResult.availableVersion));
    }
    if (updateResult.checkedAtUtc.isValid()) {
        updateLines.append(tr("Last checked (UTC): %1").arg(updateResult.checkedAtUtc.toUTC().toString(Qt::ISODate)));
    }
    if (!updateResult.message.isEmpty()) {
        updateLines.append(tr("Result: %1").arg(updateResult.message));
    }
    updateLines.append(tr("Manifest check only — no download or installation in version 0.1.5"));
    layout->addWidget(createSection(tr("Updates"), updateLines));

    layout->addWidget(createSection(
        tr("Diagnostics"),
        {tr("Logging: %1 (%2)").arg(config.loggingEnabled ? tr("Enabled") : tr("Disabled"), config.loggingLevel),
         tr("Logs: %1").arg(AppLogger::logDirectoryFor(config)),
         tr("Safe support package: available in Settings → Diagnostics")}
    ));

    layout->addStretch();
}

QGroupBox *TemplateDashboard::createSection(const QString &title, const QStringList &lines) const
{
    auto *groupBox = new QGroupBox(title);
    auto *layout = new QVBoxLayout(groupBox);
    for (const QString &line : lines) {
        auto *label = new QLabel(line, groupBox);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(label);
    }
    return groupBox;
}
