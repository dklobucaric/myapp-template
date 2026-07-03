#include "workspace/TemplateDashboard.h"

#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QSysInfo>
#include <QVBoxLayout>

#include "core/AppLogger.h"

TemplateDashboard::TemplateDashboard(
    const AppConfig &config,
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
        tr("Reusable Desktop Application Template · %1 profile")
            .arg(config.environment),
        this
    );
    subtitleLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);

    if (!warnings.isEmpty()) {
        layout->addWidget(createSection(tr("Configuration Warnings"), warnings));
    }

    layout->addWidget(createSection(
        tr("Application"),
        {
            tr("Name: %1").arg(config.appName),
            tr("Product ID: %1").arg(config.productId),
            tr("Version: %1").arg(config.version),
            tr("Platform: %1").arg(QSysInfo::prettyProductName()),
            tr("Environment: %1").arg(config.environment)
        }
    ));

    layout->addWidget(createSection(
        tr("Configuration"),
        {
            config.userConfigCreated
                ? tr("Status: Created initial user config")
                : tr("Status: Loaded user config"),
            tr("User config: %1").arg(config.userConfigPath),
            tr("Theme: %1 · Accent: %2")
                .arg(config.theme, config.accentColor),
            tr("Window: %1 × %2")
                .arg(config.windowWidth)
                .arg(config.windowHeight)
        }
    ));

    layout->addWidget(createSection(
        tr("License"),
        {
            tr("Status: Not configured"),
            tr("License portal: %1").arg(config.licensePortalUrl),
            tr("License API: %1").arg(config.licenseApiBaseUrl),
            tr("LicenseManager integration: planned for version 0.1.5")
        }
    ));

    layout->addWidget(createSection(
        tr("Updates"),
        {
            tr("Channel: %1").arg(config.updateChannel),
            tr("CDN: %1").arg(config.cdnBaseUrl),
            tr("Automatic checks: every %1 minutes")
                .arg(config.updateCheckIntervalMinutes),
            tr("UpdateManager integration: planned for version 0.1.4")
        }
    ));

    layout->addWidget(createSection(
        tr("Diagnostics"),
        {
            tr("Logging: %1 (%2)")
                .arg(config.loggingEnabled ? tr("Enabled") : tr("Disabled"), config.loggingLevel),
            tr("Logs: %1").arg(AppLogger::logDirectoryFor(config)),
            tr("Safe support package: available in Settings → Diagnostics")
        }
    ));

    layout->addStretch();
}

QGroupBox *TemplateDashboard::createSection(
    const QString &title,
    const QStringList &lines
) const
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
