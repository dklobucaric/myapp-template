#pragma once

#include <QMainWindow>

#include "core/ConfigManager.h"
#include "updates/UpdateManager.h"

class QAction;
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
    void rebuildDashboard();
    void applyConfiguration(const AppConfig &config);
    void refreshStatusBar();
    void showUpdateResultMessage(const UpdateCheckResult &result);

    void showSettingsDialog();
    void checkForUpdates();
    void showLicensePlaceholder();
    void showAboutDialog();

    AppConfig m_config;
    QStringList m_configurationWarnings;
    UpdateManager *m_updateManager = nullptr;
    UpdateCheckResult m_updateResult;
    bool m_manualUpdateCheck = false;

    QAction *m_checkUpdatesAction = nullptr;
    QLabel *m_licenseIndicator = nullptr;
    QLabel *m_updateIndicator = nullptr;
    QLabel *m_versionIndicator = nullptr;
};
