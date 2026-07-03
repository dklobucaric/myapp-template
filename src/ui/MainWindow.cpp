#include "ui/MainWindow.h"

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>

#include "core/AppLogger.h"
#include "core/ThemeManager.h"
#include "settings/SettingsDialog.h"
#include "workspace/TemplateDashboard.h"

namespace
{
QString updateIndicatorText(const UpdateCheckResult &result, const AppConfig &config)
{
    switch (result.status) {
    case UpdateStatus::Checking:
        return QObject::tr("Updates: Checking…");
    case UpdateStatus::UpdateAvailable:
        return QObject::tr("Updates: %1 available").arg(result.availableVersion);
    case UpdateStatus::UpToDate:
        return QObject::tr("Updates: Up to date");
    case UpdateStatus::NetworkError:
        return QObject::tr("Updates: Check failed");
    case UpdateStatus::InvalidManifest:
        return QObject::tr("Updates: Manifest invalid");
    case UpdateStatus::UnsupportedPlatform:
        return QObject::tr("Updates: Platform unsupported");
    case UpdateStatus::Disabled:
        return QObject::tr("Updates: Disabled");
    case UpdateStatus::NotChecked:
        return QObject::tr("Updates: %1").arg(config.updateChannel);
    }
    return QObject::tr("Updates: %1").arg(config.updateChannel);
}

QString licenseIndicatorText(const LicenseCheckResult &result)
{
    switch (result.status) {
    case LicenseStatus::Checking:
        return QObject::tr("License: Checking…");
    case LicenseStatus::Active:
        return QObject::tr("License: Active");
    case LicenseStatus::Grace:
        return QObject::tr("License: Grace");
    case LicenseStatus::Expired:
        return QObject::tr("License: Expired");
    case LicenseStatus::Revoked:
        return QObject::tr("License: Revoked");
    case LicenseStatus::OfflineGrace:
        return QObject::tr("License: Offline grace");
    case LicenseStatus::VerificationRequired:
        return QObject::tr("License: Verify online");
    case LicenseStatus::NetworkError:
        return QObject::tr("License: Check failed");
    case LicenseStatus::InvalidResponse:
        return QObject::tr("License: Response invalid");
    case LicenseStatus::Disabled:
        return QObject::tr("License: Disabled");
    case LicenseStatus::NotChecked:
        return QObject::tr("License: Not checked");
    }
    return QObject::tr("License: Unknown");
}

}

MainWindow::MainWindow(
    const AppConfig &config,
    const QStringList &configurationWarnings,
    QWidget *parent
)
    : QMainWindow(parent)
    , m_config(config)
    , m_configurationWarnings(configurationWarnings)
    , m_updateManager(new UpdateManager(m_config, this))
    , m_licenseManager(new LicenseManager(m_config, this))
    , m_updateResult(m_updateManager->lastResult())
    , m_licenseResult(m_licenseManager->lastResult())
{
    ThemeManager::apply(m_config);

    setWindowTitle(m_config.appName);
    resize(m_config.windowWidth, m_config.windowHeight);

    createMenuBar();
    createStatusBar();
    rebuildDashboard();

    connect(m_updateManager, &UpdateManager::checkStarted, this, [this] {
        m_updateResult = m_updateManager->lastResult();
        refreshStatusBar();
        rebuildDashboard();
        if (m_checkUpdatesAction != nullptr) {
            m_checkUpdatesAction->setEnabled(false);
        }
    });
    connect(m_updateManager, &UpdateManager::checkCompleted, this, [this](const UpdateCheckResult &result) {
        m_updateResult = result;
        refreshStatusBar();
        rebuildDashboard();
        if (m_checkUpdatesAction != nullptr) {
            m_checkUpdatesAction->setEnabled(true);
        }
        if (m_manualUpdateCheck) {
            m_manualUpdateCheck = false;
            showUpdateResultMessage(result);
        }
    });

    connect(m_licenseManager, &LicenseManager::checkStarted, this, [this] {
        m_licenseResult = m_licenseManager->lastResult();
        refreshStatusBar();
        rebuildDashboard();
        if (m_checkLicenseAction != nullptr) {
            m_checkLicenseAction->setEnabled(false);
        }
    });
    connect(m_licenseManager, &LicenseManager::checkCompleted, this, [this](const LicenseCheckResult &result) {
        m_licenseResult = result;
        refreshStatusBar();
        rebuildDashboard();
        if (m_checkLicenseAction != nullptr) {
            m_checkLicenseAction->setEnabled(true);
        }
        if (m_manualLicenseCheck) {
            m_manualLicenseCheck = false;
            showLicenseResultMessage(result);
        }
    });

    statusBar()->setVisible(m_config.showStatusBar);
    QTimer::singleShot(250, this, [this] { m_updateManager->startAutomaticChecks(); });
    QTimer::singleShot(400, this, [this] { m_licenseManager->startAutomaticChecks(); });
}

void MainWindow::createMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *newAction = fileMenu->addAction(tr("&New"));
    newAction->setEnabled(false);

    auto *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setEnabled(false);

    auto *saveAction = fileMenu->addAction(tr("&Save"));
    saveAction->setEnabled(false);

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, [this] { close(); });

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    auto *undoAction = editMenu->addAction(tr("&Undo"));
    undoAction->setEnabled(false);
    auto *redoAction = editMenu->addAction(tr("&Redo"));
    redoAction->setEnabled(false);

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    auto *settingsAction = settingsMenu->addAction(tr("&Settings..."));
    connect(settingsAction, &QAction::triggered, this, [this] { showSettingsDialog(); });

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    m_checkUpdatesAction = helpMenu->addAction(tr("Check for &Updates"));
    connect(m_checkUpdatesAction, &QAction::triggered, this, [this] { checkForUpdates(); });

    m_checkLicenseAction = helpMenu->addAction(tr("Check &License Status"));
    connect(m_checkLicenseAction, &QAction::triggered, this, [this] { checkLicenseStatus(); });

    helpMenu->addSeparator();
    auto *aboutAction = helpMenu->addAction(tr("&About %1").arg(m_config.appName));
    connect(aboutAction, &QAction::triggered, this, [this] { showAboutDialog(); });
}

void MainWindow::createStatusBar()
{
    m_licenseIndicator = new QLabel(this);
    m_licenseIndicator->setToolTip(tr("Checks the configured license server. The server controls license duration and device limits."));

    m_updateIndicator = new QLabel(this);
    m_updateIndicator->setToolTip(tr("Checks the active CDN manifest. This version does not download or install updates."));

    m_versionIndicator = new QLabel(this);
    statusBar()->addPermanentWidget(m_licenseIndicator);
    statusBar()->addPermanentWidget(m_updateIndicator);
    statusBar()->addPermanentWidget(m_versionIndicator);
    refreshStatusBar();
}

void MainWindow::rebuildDashboard()
{
    QWidget *previousWorkspace = takeCentralWidget();
    if (previousWorkspace != nullptr) {
        previousWorkspace->deleteLater();
    }

    setCentralWidget(new TemplateDashboard(
        m_config,
        m_updateResult,
        m_licenseResult,
        m_configurationWarnings,
        this
    ));
}

void MainWindow::applyConfiguration(const AppConfig &config)
{
    m_config = config;
    AppLogger::configure(m_config);
    ThemeManager::apply(m_config);
    m_updateManager->setConfig(m_config);
    m_licenseManager->setConfig(m_config);

    setWindowTitle(m_config.appName);
    statusBar()->setVisible(m_config.showStatusBar);
    refreshStatusBar();
    rebuildDashboard();
}

void MainWindow::refreshStatusBar()
{
    statusBar()->showMessage(
        m_configurationWarnings.isEmpty() ? tr("Ready") : tr("Ready with configuration warnings")
    );
    m_licenseIndicator->setText(licenseIndicatorText(m_licenseResult));
    m_updateIndicator->setText(updateIndicatorText(m_updateResult, m_config));
    m_versionIndicator->setText(tr("v%1").arg(m_config.version));
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dialog(
        m_config,
        [this](const AppConfig &previewConfig) { applyConfiguration(previewConfig); },
        this
    );
    dialog.exec();
}

void MainWindow::checkForUpdates()
{
    if (m_updateManager->isChecking()) {
        return;
    }
    m_manualUpdateCheck = true;
    m_updateManager->checkNow();
}

void MainWindow::showUpdateResultMessage(const UpdateCheckResult &result)
{
    QString title = tr("Updates");
    QString text;
    switch (result.status) {
    case UpdateStatus::UpdateAvailable:
        title = tr("Update Available");
        text = tr("Current version: %1\nAvailable version: %2\n\n%3\n\n"
                  "Downloading and installation are intentionally not part of this version.")
                   .arg(result.currentVersion, result.availableVersion, result.message);
        QMessageBox::information(this, title, text);
        return;
    case UpdateStatus::UpToDate:
        text = tr("Version %1 is up to date on the %2 channel.").arg(result.currentVersion, m_config.updateChannel);
        QMessageBox::information(this, title, text);
        return;
    case UpdateStatus::Disabled:
        QMessageBox::information(this, title, result.message);
        return;
    case UpdateStatus::NetworkError:
    case UpdateStatus::InvalidManifest:
    case UpdateStatus::UnsupportedPlatform:
        QMessageBox::warning(this, tr("Update Check Failed"), result.message);
        return;
    case UpdateStatus::NotChecked:
    case UpdateStatus::Checking:
        return;
    }
}

void MainWindow::checkLicenseStatus()
{
    if (m_licenseManager->isChecking()) {
        return;
    }
    m_manualLicenseCheck = true;
    m_licenseManager->checkNow();
}

void MainWindow::showLicenseResultMessage(const LicenseCheckResult &result)
{
    const QString title = tr("License Status");
    QString text = tr("Status: %1\n\n%2")
        .arg(LicenseManager::statusText(result.status), result.message);

    if (!result.entitlement.licenseId.isEmpty()) {
        text += tr("\n\nSerial: %1\nPlan: %2\nDuration: %3 days\nDevices: %4 / %5")
            .arg(
                LicenseManager::redactedSerial(result.entitlement.serial),
                result.entitlement.planName,
                QString::number(result.entitlement.durationDays),
                QString::number(result.entitlement.activeDeviceCount),
                QString::number(result.entitlement.deviceLimit)
            );
    }
    if (result.entitlement.expiresAtUtc.isValid()) {
        text += tr("\nExpires (server): %1")
            .arg(result.entitlement.expiresAtUtc.toUTC().toString(Qt::ISODate));
    }
    if (result.lastVerifiedAtUtc.isValid()) {
        text += tr("\nLast verified (server): %1")
            .arg(result.lastVerifiedAtUtc.toUTC().toString(Qt::ISODate));
    }

    if (result.status == LicenseStatus::Expired
        || result.status == LicenseStatus::Revoked
        || result.status == LicenseStatus::InvalidResponse
        || result.status == LicenseStatus::NetworkError
        || result.status == LicenseStatus::VerificationRequired) {
        QMessageBox::warning(this, title, text);
        return;
    }
    QMessageBox::information(this, title, text);
}

void MainWindow::showAboutDialog()
{
    AppLogger::debug(QStringLiteral("app"), QStringLiteral("About dialog opened."));
    QMessageBox::about(
        this,
        tr("About %1").arg(m_config.appName),
        tr("<h2>%1</h2><p>Version %2</p><p>Reusable desktop application shell built with "
           "C++20, Qt 6 Widgets and CMake.</p><p><b>Product ID:</b> %3</p><p><b>Environment:</b> %4</p>")
            .arg(m_config.appName, m_config.version, m_config.productId, m_config.environment)
    );
}
