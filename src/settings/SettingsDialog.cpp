#include "settings/SettingsDialog.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace
{
constexpr auto kSchemaVersion = 1;

QJsonValue nestedValue(
    const QJsonObject &object,
    const QString &section,
    const QString &key
)
{
    return object.value(section).toObject().value(key);
}

void removeValue(
    QJsonObject &object,
    const QString &section,
    const QString &key
)
{
    QJsonObject sectionObject = object.value(section).toObject();
    sectionObject.remove(key);
    object.insert(section, sectionObject);
}

void setOrRemoveOverride(
    QJsonObject &overrides,
    const QJsonObject &baseline,
    const QString &section,
    const QString &key,
    const QJsonValue &value
)
{
    const QJsonValue baselineValue = nestedValue(baseline, section, key);
    if (baselineValue == value) {
        removeValue(overrides, section, key);
        return;
    }

    QJsonObject sectionObject = overrides.value(section).toObject();
    sectionObject.insert(key, value);
    overrides.insert(section, sectionObject);
}

void removeSection(QJsonObject &overrides, const QString &section)
{
    overrides.insert(section, QJsonObject{});
}

QWidget *createPageContainer(QWidget *parent)
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);
    return page;
}

QLabel *createHint(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("color: palette(mid);"));
    return label;
}

bool isValidWebUrl(const QString &value)
{
    const QUrl url(value);
    return url.isValid()
        && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))
        && !url.host().isEmpty();
}
} // namespace

SettingsDialog::SettingsDialog(
    const AppConfig &config,
    PreviewCallback previewCallback,
    QWidget *parent
)
    : QDialog(parent)
    , m_appliedConfig(config)
    , m_userOverrides(config.userOverrides)
    , m_configDirectory(QFileInfo(config.userConfigPath).absolutePath())
    , m_previewCallback(std::move(previewCallback))
{
    setWindowTitle(tr("Settings"));
    resize(900, 620);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    auto *contentLayout = new QHBoxLayout();

    m_navigation = new QListWidget(this);
    m_navigation->setMinimumWidth(180);
    m_navigation->setMaximumWidth(240);

    m_pages = new QStackedWidget(this);

    addPage(tr("General"), createGeneralPage());
    addPage(tr("Appearance"), createAppearancePage());
    addPage(tr("Layout"), createLayoutPage());
    addPage(tr("Updates & CDN"), createUpdatesPage());
    addPage(tr("License Server"), createLicensePage());
    addPage(tr("Diagnostics"), createDiagnosticsPage());
    addPage(tr("Advanced"), createAdvancedPage());

    contentLayout->addWidget(m_navigation);
    contentLayout->addWidget(m_pages, 1);
    mainLayout->addLayout(contentLayout, 1);

    auto *buttonRow = new QHBoxLayout();
    m_resetPageButton = new QPushButton(tr("Reset Current Page"), this);
    auto *resetAllButton = new QPushButton(tr("Reset All Settings"), this);
    buttonRow->addWidget(m_resetPageButton);
    buttonRow->addWidget(resetAllButton);
    buttonRow->addStretch();

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this
    );
    buttonRow->addWidget(buttonBox);
    mainLayout->addLayout(buttonRow);

    connect(
        m_navigation,
        &QListWidget::currentRowChanged,
        m_pages,
        &QStackedWidget::setCurrentIndex
    );
    connect(m_resetPageButton, &QPushButton::clicked, this, [this] {
        resetCurrentPage();
    });
    connect(m_navigation, &QListWidget::currentRowChanged, this, [this](int row) {
        m_resetPageButton->setEnabled(row >= 1 && row <= 5);
    });
    connect(resetAllButton, &QPushButton::clicked, this, [this] {
        resetAllSettings();
    });
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this] {
        applyAndKeepOpen();
    });
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, [this] {
        applyAndAccept();
    });
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, [this] {
        cancelAndReject();
    });
    connect(this, &QDialog::rejected, this, [this] {
        if (m_previewCallback) {
            m_previewCallback(m_appliedConfig);
        }
    });

    m_navigation->setCurrentRow(0);
    loadControlsFromConfig(m_appliedConfig);
}

const AppConfig &SettingsDialog::appliedConfig() const
{
    return m_appliedConfig;
}

bool SettingsDialog::hasAppliedChanges() const
{
    return m_hasAppliedChanges;
}

QWidget *SettingsDialog::createGeneralPage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());

    auto *form = new QFormLayout();
    m_environmentValue = new QLabel(page);
    m_applicationValue = new QLabel(page);
    m_productIdValue = new QLabel(page);
    m_configPathEdit = new QLineEdit(page);
    m_configPathEdit->setReadOnly(true);

    form->addRow(tr("Environment:"), m_environmentValue);
    form->addRow(tr("Application:"), m_applicationValue);
    form->addRow(tr("Product ID:"), m_productIdValue);
    form->addRow(tr("User config:"), m_configPathEdit);

    auto *openFolderButton = new QPushButton(tr("Open Config Folder"), page);
    connect(openFolderButton, &QPushButton::clicked, this, [this] {
        openConfigFolder();
    });

    layout->addLayout(form);
    layout->addWidget(openFolderButton, 0, Qt::AlignLeft);
    layout->addWidget(createHint(
        tr("Settings are stored as local overrides. Environment profile values remain "
           "active until you explicitly change a setting."),
        page
    ));
    layout->addStretch();

    return page;
}

QWidget *SettingsDialog::createAppearancePage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    m_themeCombo = new QComboBox(page);
    m_themeCombo->addItem(tr("System"), QStringLiteral("system"));
    m_themeCombo->addItem(tr("Light"), QStringLiteral("light"));
    m_themeCombo->addItem(tr("Dark"), QStringLiteral("dark"));

    m_accentColorEdit = new QLineEdit(page);
    m_accentColorEdit->setPlaceholderText(QStringLiteral("#3B82F6"));
    m_chooseAccentButton = new QPushButton(tr("Choose…"), page);
    auto *accentRow = new QHBoxLayout();
    accentRow->setContentsMargins(0, 0, 0, 0);
    accentRow->addWidget(m_accentColorEdit, 1);
    accentRow->addWidget(m_chooseAccentButton);
    auto *accentWidget = new QWidget(page);
    accentWidget->setLayout(accentRow);

    m_fontScaleSpin = new QDoubleSpinBox(page);
    m_fontScaleSpin->setRange(0.75, 2.0);
    m_fontScaleSpin->setSingleStep(0.05);
    m_fontScaleSpin->setDecimals(2);
    m_fontScaleSpin->setSuffix(tr(" ×"));

    form->addRow(tr("Theme:"), m_themeCombo);
    form->addRow(tr("Accent color:"), accentWidget);
    form->addRow(tr("Font scale:"), m_fontScaleSpin);

    layout->addLayout(form);
    layout->addWidget(createHint(
        tr("Theme, accent color and font scale preview immediately. Cancel restores the "
           "last applied appearance."),
        page
    ));
    layout->addStretch();

    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this] {
        previewCurrentControls();
    });
    connect(m_accentColorEdit, &QLineEdit::textEdited, this, [this] {
        previewCurrentControls();
    });
    connect(m_fontScaleSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
        previewCurrentControls();
    });
    connect(m_chooseAccentButton, &QPushButton::clicked, this, [this] {
        const QColor selected = QColorDialog::getColor(
            QColor(m_accentColorEdit->text()),
            this,
            tr("Choose Accent Color")
        );
        if (selected.isValid()) {
            m_accentColorEdit->setText(selected.name(QColor::HexRgb));
            previewCurrentControls();
        }
    });

    return page;
}

QWidget *SettingsDialog::createLayoutPage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    m_windowWidthSpin = new QSpinBox(page);
    m_windowWidthSpin->setRange(640, 7680);
    m_windowWidthSpin->setSuffix(tr(" px"));

    m_windowHeightSpin = new QSpinBox(page);
    m_windowHeightSpin->setRange(480, 4320);
    m_windowHeightSpin->setSuffix(tr(" px"));

    m_showStatusBarCheck = new QCheckBox(tr("Show Status Bar"), page);

    form->addRow(tr("Initial width:"), m_windowWidthSpin);
    form->addRow(tr("Initial height:"), m_windowHeightSpin);
    form->addRow(QString(), m_showStatusBarCheck);

    layout->addLayout(form);
    layout->addWidget(createHint(
        tr("Window dimensions are saved for the next application start. Status Bar "
           "visibility previews immediately."),
        page
    ));
    layout->addStretch();

    connect(m_windowWidthSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this] {
        previewCurrentControls();
    });
    connect(m_windowHeightSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this] {
        previewCurrentControls();
    });
    connect(m_showStatusBarCheck, &QCheckBox::toggled, this, [this] {
        previewCurrentControls();
    });

    return page;
}

QWidget *SettingsDialog::createUpdatesPage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    m_updateChannelCombo = new QComboBox(page);
    m_updateChannelCombo->addItem(tr("Development"), QStringLiteral("development"));
    m_updateChannelCombo->addItem(tr("Beta"), QStringLiteral("beta"));
    m_updateChannelCombo->addItem(tr("Stable"), QStringLiteral("stable"));

    m_automaticChecksCheck = new QCheckBox(tr("Check for updates automatically"), page);

    m_updateIntervalSpin = new QSpinBox(page);
    m_updateIntervalSpin->setRange(5, 24 * 60);
    m_updateIntervalSpin->setSuffix(tr(" minutes"));

    m_autoDownloadCheck = new QCheckBox(tr("Download updates automatically"), page);

    m_cdnBaseUrlEdit = new QLineEdit(page);
    m_cdnBaseUrlEdit->setClearButtonEnabled(true);

    form->addRow(tr("Update channel:"), m_updateChannelCombo);
    form->addRow(QString(), m_automaticChecksCheck);
    form->addRow(tr("Check interval:"), m_updateIntervalSpin);
    form->addRow(QString(), m_autoDownloadCheck);
    form->addRow(tr("CDN Base URL:"), m_cdnBaseUrlEdit);

    layout->addLayout(form);
    layout->addWidget(createHint(
        tr("This version persists update preferences and the CDN endpoint. Connection tests "
           "arrive with UpdateManager."),
        page
    ));
    layout->addStretch();

    connect(m_updateChannelCombo, &QComboBox::currentIndexChanged, this, [this] {
        previewCurrentControls();
    });
    connect(m_automaticChecksCheck, &QCheckBox::toggled, this, [this] {
        previewCurrentControls();
    });
    connect(m_updateIntervalSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this] {
        previewCurrentControls();
    });
    connect(m_autoDownloadCheck, &QCheckBox::toggled, this, [this] {
        previewCurrentControls();
    });
    connect(m_cdnBaseUrlEdit, &QLineEdit::textEdited, this, [this] {
        previewCurrentControls();
    });

    return page;
}

QWidget *SettingsDialog::createLicensePage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    m_licensePortalUrlEdit = new QLineEdit(page);
    m_licensePortalUrlEdit->setClearButtonEnabled(true);
    m_licenseApiUrlEdit = new QLineEdit(page);
    m_licenseApiUrlEdit->setClearButtonEnabled(true);

    form->addRow(tr("License Portal URL:"), m_licensePortalUrlEdit);
    form->addRow(tr("License API URL:"), m_licenseApiUrlEdit);

    layout->addLayout(form);
    layout->addWidget(createHint(
        tr("URLs are persisted as local overrides for development and testing. License server "
           "connection checks arrive with LicenseManager."),
        page
    ));
    layout->addStretch();

    connect(m_licensePortalUrlEdit, &QLineEdit::textEdited, this, [this] {
        previewCurrentControls();
    });
    connect(m_licenseApiUrlEdit, &QLineEdit::textEdited, this, [this] {
        previewCurrentControls();
    });

    return page;
}

QWidget *SettingsDialog::createDiagnosticsPage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    m_loggingEnabledCheck = new QCheckBox(tr("Enable logging"), page);
    m_loggingLevelCombo = new QComboBox(page);
    m_loggingLevelCombo->addItem(QStringLiteral("error"));
    m_loggingLevelCombo->addItem(QStringLiteral("warning"));
    m_loggingLevelCombo->addItem(QStringLiteral("info"));
    m_loggingLevelCombo->addItem(QStringLiteral("debug"));

    form->addRow(QString(), m_loggingEnabledCheck);
    form->addRow(tr("Log level:"), m_loggingLevelCombo);

    layout->addLayout(form);
    layout->addWidget(createHint(
        tr("Logging preferences are persisted now. Structured log files and diagnostics export "
           "arrive in the next diagnostics milestone."),
        page
    ));
    layout->addStretch();

    connect(m_loggingEnabledCheck, &QCheckBox::toggled, this, [this] {
        previewCurrentControls();
    });
    connect(m_loggingLevelCombo, &QComboBox::currentIndexChanged, this, [this] {
        previewCurrentControls();
    });

    return page;
}

QWidget *SettingsDialog::createAdvancedPage()
{
    auto *page = createPageContainer(this);
    auto *layout = static_cast<QVBoxLayout *>(page->layout());
    auto *form = new QFormLayout();

    auto *environmentValue = new QLabel(m_appliedConfig.environment, page);
    auto *configPathEdit = new QLineEdit(m_appliedConfig.userConfigPath, page);
    configPathEdit->setReadOnly(true);

    form->addRow(tr("Active environment:"), environmentValue);
    form->addRow(tr("Config file:"), configPathEdit);

    auto *openFolderButton = new QPushButton(tr("Open Config Folder"), page);
    connect(openFolderButton, &QPushButton::clicked, this, [this] {
        openConfigFolder();
    });

    layout->addLayout(form);
    layout->addWidget(openFolderButton, 0, Qt::AlignLeft);
    layout->addWidget(createHint(
        tr("Configuration precedence: default config → app profile → environment profile → "
           "user config. Reset actions remove only local overrides."),
        page
    ));
    layout->addStretch();

    return page;
}

void SettingsDialog::addPage(const QString &category, QWidget *page)
{
    m_navigation->addItem(category);
    m_pages->addWidget(page);
}

void SettingsDialog::loadControlsFromConfig(const AppConfig &config)
{
    m_loadingControls = true;

    m_environmentValue->setText(config.environment);
    m_applicationValue->setText(config.appName);
    m_productIdValue->setText(config.productId);
    m_configPathEdit->setText(config.userConfigPath);

    const auto setComboData = [](QComboBox *combo, const QString &value) {
        const int index = combo->findData(value);
        combo->setCurrentIndex(index >= 0 ? index : 0);
    };

    setComboData(m_themeCombo, config.theme);
    m_accentColorEdit->setText(config.accentColor);
    m_fontScaleSpin->setValue(config.fontScale);

    m_windowWidthSpin->setValue(config.windowWidth);
    m_windowHeightSpin->setValue(config.windowHeight);
    m_showStatusBarCheck->setChecked(config.showStatusBar);

    setComboData(m_updateChannelCombo, config.updateChannel);
    m_automaticChecksCheck->setChecked(config.automaticUpdateChecks);
    m_updateIntervalSpin->setValue(config.updateCheckIntervalMinutes);
    m_autoDownloadCheck->setChecked(config.autoDownloadUpdates);
    m_cdnBaseUrlEdit->setText(config.cdnBaseUrl);

    m_licensePortalUrlEdit->setText(config.licensePortalUrl);
    m_licenseApiUrlEdit->setText(config.licenseApiBaseUrl);

    m_loggingEnabledCheck->setChecked(config.loggingEnabled);
    setComboData(m_loggingLevelCombo, config.loggingLevel);

    m_loadingControls = false;
}

void SettingsDialog::previewCurrentControls()
{
    if (m_loadingControls) {
        return;
    }

    rebuildUserOverridesFromControls();
    if (m_previewCallback) {
        m_previewCallback(previewConfig());
    }
}

void SettingsDialog::rebuildUserOverridesFromControls()
{
    m_userOverrides.insert(QStringLiteral("schemaVersion"), kSchemaVersion);

    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("appearance"),
        QStringLiteral("theme"),
        m_themeCombo->currentData().toString()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("appearance"),
        QStringLiteral("accentColor"),
        m_accentColorEdit->text().trimmed()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("appearance"),
        QStringLiteral("fontScale"),
        m_fontScaleSpin->value()
    );

    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("layout"),
        QStringLiteral("windowWidth"),
        m_windowWidthSpin->value()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("layout"),
        QStringLiteral("windowHeight"),
        m_windowHeightSpin->value()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("layout"),
        QStringLiteral("showStatusBar"),
        m_showStatusBarCheck->isChecked()
    );

    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("updates"),
        QStringLiteral("channel"),
        m_updateChannelCombo->currentData().toString()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("updates"),
        QStringLiteral("automaticChecks"),
        m_automaticChecksCheck->isChecked()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("updates"),
        QStringLiteral("checkIntervalMinutes"),
        m_updateIntervalSpin->value()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("updates"),
        QStringLiteral("autoDownload"),
        m_autoDownloadCheck->isChecked()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("services"),
        QStringLiteral("cdnBaseUrl"),
        m_cdnBaseUrlEdit->text().trimmed()
    );

    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("services"),
        QStringLiteral("licensePortalUrl"),
        m_licensePortalUrlEdit->text().trimmed()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("services"),
        QStringLiteral("licenseApiBaseUrl"),
        m_licenseApiUrlEdit->text().trimmed()
    );

    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("diagnostics"),
        QStringLiteral("loggingEnabled"),
        m_loggingEnabledCheck->isChecked()
    );
    setOrRemoveOverride(
        m_userOverrides,
        m_appliedConfig.baselineConfig,
        QStringLiteral("diagnostics"),
        QStringLiteral("loggingLevel"),
        m_loggingLevelCombo->currentText()
    );
}

void SettingsDialog::resetCurrentPage()
{
    switch (m_navigation->currentRow()) {
    case 1:
        removeSection(m_userOverrides, QStringLiteral("appearance"));
        break;
    case 2:
        removeSection(m_userOverrides, QStringLiteral("layout"));
        break;
    case 3:
        removeSection(m_userOverrides, QStringLiteral("updates"));
        removeValue(m_userOverrides, QStringLiteral("services"), QStringLiteral("cdnBaseUrl"));
        break;
    case 4:
        removeValue(m_userOverrides, QStringLiteral("services"), QStringLiteral("licensePortalUrl"));
        removeValue(m_userOverrides, QStringLiteral("services"), QStringLiteral("licenseApiBaseUrl"));
        break;
    case 5:
        removeSection(m_userOverrides, QStringLiteral("diagnostics"));
        break;
    default:
        return;
    }

    const AppConfig defaults = previewConfig();
    loadControlsFromConfig(defaults);
    previewCurrentControls();
}

void SettingsDialog::resetAllSettings()
{
    const auto answer = QMessageBox::question(
        this,
        tr("Reset All Settings"),
        tr("Remove all local configuration overrides and return to the active environment defaults?")
    );
    if (answer != QMessageBox::Yes) {
        return;
    }

    m_userOverrides = ConfigManager::emptyUserOverrides();
    const AppConfig defaults = previewConfig();
    loadControlsFromConfig(defaults);
    previewCurrentControls();
}

void SettingsDialog::openConfigFolder() const
{
    const QFileInfo configFile(m_appliedConfig.userConfigPath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(configFile.absolutePath()));
}

bool SettingsDialog::persistChanges()
{
    const QColor accentColor(m_accentColorEdit->text().trimmed());
    if (!accentColor.isValid()) {
        QMessageBox::warning(
            this,
            tr("Invalid Accent Color"),
            tr("Enter a valid color value such as #3B82F6.")
        );
        return false;
    }

    const QList<QPair<QString, QString>> endpointValues{
        {tr("CDN Base URL"), m_cdnBaseUrlEdit->text().trimmed()},
        {tr("License Portal URL"), m_licensePortalUrlEdit->text().trimmed()},
        {tr("License API URL"), m_licenseApiUrlEdit->text().trimmed()}
    };
    for (const auto &[label, value] : endpointValues) {
        if (!isValidWebUrl(value)) {
            QMessageBox::warning(
                this,
                tr("Invalid Service URL"),
                tr("%1 must be a valid HTTP or HTTPS URL.").arg(label)
            );
            return false;
        }
    }

    rebuildUserOverridesFromControls();

    QString errorMessage;
    if (!ConfigManager::saveUserOverrides(
            m_userOverrides,
            m_configDirectory,
            &errorMessage
        )) {
        QMessageBox::critical(
            this,
            tr("Could Not Save Settings"),
            tr("The configuration file could not be saved.\n\n%1").arg(errorMessage)
        );
        return false;
    }

    const ConfigLoadResult result = ConfigManager::load(
        m_appliedConfig.environment,
        m_configDirectory
    );
    if (!result.isUsable()) {
        QMessageBox::critical(
            this,
            tr("Could Not Reload Settings"),
            tr("The saved configuration could not be reloaded safely.")
        );
        return false;
    }

    m_appliedConfig = result.config;
    m_userOverrides = m_appliedConfig.userOverrides;
    m_hasAppliedChanges = true;
    loadControlsFromConfig(m_appliedConfig);

    if (m_previewCallback) {
        m_previewCallback(m_appliedConfig);
    }

    return true;
}

void SettingsDialog::applyAndKeepOpen()
{
    persistChanges();
}

void SettingsDialog::applyAndAccept()
{
    if (persistChanges()) {
        accept();
    }
}

void SettingsDialog::cancelAndReject()
{
    reject();
}

AppConfig SettingsDialog::previewConfig() const
{
    return ConfigManager::configWithUserOverrides(m_appliedConfig, m_userOverrides);
}
