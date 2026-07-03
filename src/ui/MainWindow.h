#pragma once

#include <QMainWindow>

#include "core/ConfigManager.h"

class QLabel;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(
        const AppConfig &config,
        const QStringList &configurationWarnings = {},
        QWidget *parent = nullptr
    );

private:
    void createMenuBar();
    void createStatusBar();

    void showSettingsDialog();
    void showUpdatePlaceholder();
    void showLicensePlaceholder();
    void showAboutDialog();

    AppConfig m_config;
    QStringList m_configurationWarnings;

    QLabel *m_licenseIndicator = nullptr;
    QLabel *m_updateIndicator = nullptr;
    QLabel *m_versionIndicator = nullptr;
};
