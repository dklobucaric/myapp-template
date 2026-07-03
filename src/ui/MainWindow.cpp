#include "ui/MainWindow.h"

#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "workspace/TemplateDashboard.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("MyAppTemplate"));
    resize(1200, 800);

    setCentralWidget(new TemplateDashboard(this));

    createMenuBar();
    createStatusBar();
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
        showSettingsPlaceholder();
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

    auto *aboutAction = helpMenu->addAction(tr("&About MyAppTemplate"));
    connect(aboutAction, &QAction::triggered, this, [this] {
        showAboutDialog();
    });
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));

    m_licenseIndicator = new QLabel(tr("License: Not configured"), this);
    m_licenseIndicator->setToolTip(
        tr("License status will be connected in version 0.1.6.")
    );

    m_updateIndicator = new QLabel(tr("Updates: Development"), this);
    m_updateIndicator->setToolTip(
        tr("Mock CDN integration will be connected in version 0.1.5.")
    );

    m_versionIndicator = new QLabel(
        tr("v%1").arg(QStringLiteral(MYAPP_TEMPLATE_VERSION)),
        this
    );

    statusBar()->addPermanentWidget(m_licenseIndicator);
    statusBar()->addPermanentWidget(m_updateIndicator);
    statusBar()->addPermanentWidget(m_versionIndicator);
}

void MainWindow::showSettingsPlaceholder()
{
    QMessageBox::information(
        this,
        tr("Settings"),
        tr("The Settings Dialog is planned for version 0.1.1.\n\n"
           "The default JSON configuration files already exist under "
           "assets/defaults/.")
    );
}

void MainWindow::showUpdatePlaceholder()
{
    QMessageBox::information(
        this,
        tr("Updates"),
        tr("The mock CDN is running separately during development.\n\n"
           "UpdateManager integration is planned for version 0.1.5.")
    );
}

void MainWindow::showLicensePlaceholder()
{
    QMessageBox::information(
        this,
        tr("License Status"),
        tr("The license JSON model and mock license-server response are ready.\n\n"
           "LicenseManager integration is planned for version 0.1.6.")
    );
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        tr("About MyAppTemplate"),
        tr("<h2>MyAppTemplate</h2>"
           "<p>Version %1</p>"
           "<p>Reusable desktop application shell built with "
           "C++20, Qt 6 Widgets and CMake.</p>"
           "<p><b>Product ID:</b> hr.ddlab.myapp-template</p>"
           "<p><b>Environment:</b> development</p>")
            .arg(QStringLiteral(MYAPP_TEMPLATE_VERSION))
    );
}
