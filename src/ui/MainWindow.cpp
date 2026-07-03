#include "ui/MainWindow.h"

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "settings/SettingsDialog.h"
#include "workspace/TemplateDashboard.h"

MainWindow::MainWindow(
    const AppConfig &config,
    const QStringList &configurationWarnings,
    QWidget *parent
)
    : QMainWindow(parent)
    , m_config(config)
    , m_configurationWarnings(configurationWarnings)
{
    setWindowTitle(m_config.appName);
    resize(m_config.windowWidth, m_config.windowHeight);

    setCentralWidget(new TemplateDashboard(
        m_config,
        m_configurationWarnings,
        this
    ));

    createMenuBar();
    createStatusBar();

    statusBar()->setVisible(m_config.showStatusBar);
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

    connect(exitAction, &QAction::triggered, this, [this] {
        close();
    });

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));

    auto *undoAction = editMenu->addAction(tr("&Undo"));
    undoAction->setEnabled(false);

    auto *redoAction = editMenu->addAction(tr("&Redo"));
    redoAction->setEnabled(false);

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    auto *settingsAction = settingsMenu->addAction(tr("&Settings..."));

    connect(settingsAction, &QAction::triggered, this, [this] {
        showSettingsDialog();
    });

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));

    auto *checkUpdatesAction = helpMenu->addAction(tr("Check for &Updates"));
    connect(checkUpdatesAction, &QAction::triggered, this, [this] {
        showUpdatePlaceholder();
    });

    auto *licenseStatusAction = helpMenu->addAction(tr("&License Status"));
    connect(licenseStatusAction, &QAction::triggered, this, [this] {
        showLicensePlaceholder();
    });

    helpMenu->addSeparator();

    auto *aboutAction = helpMenu->addAction(tr("&About %1").arg(m_config.appName));
    connect(aboutAction, &QAction::triggered, this, [this] {
        showAboutDialog();
    });
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(
        m_configurationWarnings.isEmpty()
            ? tr("Ready")
            : tr("Ready with configuration warnings")
    );

    m_licenseIndicator = new QLabel(tr("License: Not configured"), this);
    m_licenseIndicator->setToolTip(
        tr("License status will be connected in version 0.1.6.")
    );

    m_updateIndicator = new QLabel(
        tr("Updates: %1").arg(m_config.updateChannel),
        this
    );
    m_updateIndicator->setToolTip(
        tr("Mock CDN integration will be connected in version 0.1.5.")
    );

    m_versionIndicator = new QLabel(
        tr("v%1").arg(m_config.version),
        this
    );

    statusBar()->addPermanentWidget(m_licenseIndicator);
    statusBar()->addPermanentWidget(m_updateIndicator);
    statusBar()->addPermanentWidget(m_versionIndicator);
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dialog(m_config, this);
    dialog.exec();
}

void MainWindow::showUpdatePlaceholder()
{
    QMessageBox::information(
        this,
        tr("Updates"),
        tr("The configured development CDN is:\n%1\n\n"
           "UpdateManager integration is planned for version 0.1.5.")
            .arg(m_config.cdnBaseUrl)
    );
}

void MainWindow::showLicensePlaceholder()
{
    QMessageBox::information(
        this,
        tr("License Status"),
        tr("The configured license API is:\n%1\n\n"
           "LicenseManager integration is planned for version 0.1.6.")
            .arg(m_config.licenseApiBaseUrl)
    );
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        tr("About %1").arg(m_config.appName),
        tr("<h2>%1</h2>"
           "<p>Version %2</p>"
           "<p>Reusable desktop application shell built with "
           "C++20, Qt 6 Widgets and CMake.</p>"
           "<p><b>Product ID:</b> %3</p>"
           "<p><b>Environment:</b> %4</p>")
            .arg(
                m_config.appName,
                m_config.version,
                m_config.productId,
                m_config.environment
            )
    );
}
