#include "settings/SettingsDialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(const AppConfig &config, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    resize(820, 520);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    auto *contentLayout = new QHBoxLayout();

    m_navigation = new QListWidget(this);
    m_navigation->setMinimumWidth(170);
    m_navigation->setMaximumWidth(220);

    m_pages = new QStackedWidget(this);

    const auto addPage = [this](const QString &category, QWidget *page) {
        m_navigation->addItem(category);
        m_pages->addWidget(page);
    };

    addPage(
        tr("General"),
        createDetailsPage(
            tr("General"),
            {
                tr("Environment: %1").arg(config.environment),
                tr("Application: %1").arg(config.appName),
                tr("Product ID: %1").arg(config.productId),
                tr("The editable General settings controls will be added in a later milestone.")
            }
        )
    );

    addPage(
        tr("Appearance"),
        createDetailsPage(
            tr("Appearance"),
            {
                tr("Theme: %1").arg(config.theme),
                tr("Accent color: %1").arg(config.accentColor),
                tr("Font scale: %1").arg(config.fontScale),
                tr("Density: %1").arg(config.density)
            }
        )
    );

    addPage(
        tr("Layout"),
        createDetailsPage(
            tr("Layout"),
            {
                tr("Initial window size: %1 × %2")
                    .arg(config.windowWidth)
                    .arg(config.windowHeight),
                tr("Remember window size: %1")
                    .arg(config.rememberWindowSize ? tr("Yes") : tr("No")),
                tr("Remember window position: %1")
                    .arg(config.rememberWindowPosition ? tr("Yes") : tr("No")),
                tr("Show Status Bar: %1")
                    .arg(config.showStatusBar ? tr("Yes") : tr("No"))
            }
        )
    );

    addPage(
        tr("Updates & CDN"),
        createDetailsPage(
            tr("Updates & CDN"),
            {
                tr("Channel: %1").arg(config.updateChannel),
                tr("Automatic checks: %1")
                    .arg(config.automaticUpdateChecks ? tr("Yes") : tr("No")),
                tr("Check interval: %1 minutes").arg(config.updateCheckIntervalMinutes),
                tr("CDN Base URL: %1").arg(config.cdnBaseUrl)
            }
        )
    );

    addPage(
        tr("License Server"),
        createDetailsPage(
            tr("License Server"),
            {
                tr("Portal URL: %1").arg(config.licensePortalUrl),
                tr("API URL: %1").arg(config.licenseApiBaseUrl),
                tr("LicenseManager integration is planned for version 0.1.6.")
            }
        )
    );

    addPage(
        tr("Diagnostics"),
        createDetailsPage(
            tr("Diagnostics"),
            {
                tr("Logging enabled: %1")
                    .arg(config.loggingEnabled ? tr("Yes") : tr("No")),
                tr("Log level: %1").arg(config.loggingLevel),
                tr("Diagnostics bundle support is planned for version 0.1.4.")
            }
        )
    );

    addPage(
        tr("Advanced"),
        createDetailsPage(
            tr("Advanced"),
            {
                tr("User config file:"),
                config.userConfigPath,
                tr("Configuration precedence:"),
                tr("Default config → app profile → environment profile → user config")
            }
        )
    );

    contentLayout->addWidget(m_navigation);
    contentLayout->addWidget(m_pages, 1);
    mainLayout->addLayout(contentLayout, 1);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    connect(
        m_navigation,
        &QListWidget::currentRowChanged,
        m_pages,
        &QStackedWidget::setCurrentIndex
    );

    m_navigation->setCurrentRow(0);
}

QWidget *SettingsDialog::createDetailsPage(
    const QString &title,
    const QStringList &lines
) const
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *groupBox = new QGroupBox(title, page);
    auto *groupLayout = new QVBoxLayout(groupBox);

    for (const QString &line : lines) {
        auto *label = new QLabel(line, groupBox);
        label->setWordWrap(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        groupLayout->addWidget(label);
    }

    layout->addWidget(groupBox);
    layout->addStretch();

    return page;
}
