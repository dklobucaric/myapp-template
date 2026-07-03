#pragma once

#include <QStringList>
#include <QWidget>

#include "core/ConfigManager.h"

class QGroupBox;

class TemplateDashboard final : public QWidget
{
public:
    explicit TemplateDashboard(
        const AppConfig &config,
        const QStringList &warnings = {},
        QWidget *parent = nullptr
    );

private:
    QGroupBox *createSection(
        const QString &title,
        const QStringList &lines
    ) const;
};
