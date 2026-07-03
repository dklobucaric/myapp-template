#include "workspace/TemplateDashboard.h"

#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

TemplateDashboard::TemplateDashboard(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *titleLabel = new QLabel(tr("MyAppTemplate"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 10);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *subtitleLabel = new QLabel(
        tr("Reusable Desktop Application Template · Linux Development Baseline"),
        this
    );
    subtitleLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);

    layout->addWidget(createSection(
        tr("Application"),
        {
            tr("Name: MyAppTemplate"),
            tr("Product ID: hr.ddlab.myapp-template"),
            tr("Platform: Linux"),
            tr("Environment: development")
        }
    ));

    layout->addWidget(createSection(
        tr("Configuration"),
        {
            tr("Status: Default JSON files prepared"),
            tr("Profile: development"),
            tr("Settings Dialog: planned for version 0.1.1")
        }
    ));

    layout->addWidget(createSection(
        tr("License"),
        {
            tr("Status: Not configured"),
            tr("Mock license server: http://localhost:8089"),
            tr("LicenseManager integration: planned for version 0.1.6")
        }
    ));

    layout->addWidget(createSection(
        tr("Updates"),
        {
            tr("Channel: development"),
            tr("Mock CDN: http://localhost:8088"),
            tr("Mock update available: 0.1.1"),
            tr("UpdateManager integration: planned for version 0.1.5")
        }
    ));

    layout->addWidget(createSection(
        tr("Diagnostics"),
        {
            tr("Logging and diagnostics foundation: planned for version 0.1.4")
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
        layout->addWidget(label);
    }

    return groupBox;
}
