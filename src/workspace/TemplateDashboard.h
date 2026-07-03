#pragma once

#include <QStringList>
#include <QWidget>

#include "core/ConfigManager.h"
#include "licensing/LicenseManager.h"
#include "updates/UpdateManager.h"

class QGroupBox;

class TemplateDashboard final : public QWidget
{
public:
    explicit TemplateDashboard(
        const AppConfig &config,
        const UpdateCheckResult &updateResult = {},
        const LicenseCheckResult &licenseResult = {},
        const QStringList &warnings = {},
        QWidget *parent = nullptr
    );

private:
    QGroupBox *createSection(const QString &title, const QStringList &lines) const;
};
