#pragma once

#include <QDialog>
#include <QJsonObject>
#include <functional>

#include "core/ConfigManager.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;

/**
 * Functional editor for the user-override layer in config.json.
 *
 * The dialog never writes environment values into config.json unless the user
 * explicitly changes them. This keeps development, staging and production
 * profiles portable and predictable.
 */
class SettingsDialog final : public QDialog
{
public:
    using PreviewCallback = std::function<void(const AppConfig &)>;

    explicit SettingsDialog(
        const AppConfig &config,
        PreviewCallback previewCallback = {},
        QWidget *parent = nullptr
    );

    [[nodiscard]] const AppConfig &appliedConfig() const;
    [[nodiscard]] bool hasAppliedChanges() const;

private:
    QWidget *createGeneralPage();
    QWidget *createAppearancePage();
    QWidget *createLayoutPage();
    QWidget *createUpdatesPage();
    QWidget *createLicensePage();
    QWidget *createDiagnosticsPage();
    QWidget *createAdvancedPage();

    void addPage(const QString &category, QWidget *page);
    void loadControlsFromConfig(const AppConfig &config);
    void previewCurrentControls();
    void rebuildUserOverridesFromControls();
    void resetCurrentPage();
    void resetAllSettings();
    void openConfigFolder() const;
    void openLogsFolder();
    void createSafeSupportPackage();

    bool persistChanges();
    void applyAndKeepOpen();
    void applyAndAccept();
    void cancelAndReject();

    [[nodiscard]] AppConfig previewConfig() const;

    AppConfig m_appliedConfig;
    QJsonObject m_userOverrides;
    QString m_configDirectory;
    PreviewCallback m_previewCallback;
    bool m_hasAppliedChanges = false;
    bool m_loadingControls = false;

    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
    QPushButton *m_resetPageButton = nullptr;

    QLabel *m_environmentValue = nullptr;
    QLabel *m_applicationValue = nullptr;
    QLabel *m_productIdValue = nullptr;
    QLineEdit *m_configPathEdit = nullptr;

    QComboBox *m_themeCombo = nullptr;
    QLineEdit *m_accentColorEdit = nullptr;
    QPushButton *m_chooseAccentButton = nullptr;
    QDoubleSpinBox *m_fontScaleSpin = nullptr;
    QSpinBox *m_windowWidthSpin = nullptr;
    QSpinBox *m_windowHeightSpin = nullptr;
    QCheckBox *m_showStatusBarCheck = nullptr;

    QComboBox *m_updateChannelCombo = nullptr;
    QCheckBox *m_automaticChecksCheck = nullptr;
    QSpinBox *m_updateIntervalSpin = nullptr;
    QCheckBox *m_autoDownloadCheck = nullptr;
    QLineEdit *m_cdnBaseUrlEdit = nullptr;

    QLineEdit *m_licensePortalUrlEdit = nullptr;
    QLineEdit *m_licenseApiUrlEdit = nullptr;
    QCheckBox *m_automaticLicenseChecksCheck = nullptr;
    QSpinBox *m_licenseIntervalSpin = nullptr;

    QCheckBox *m_loggingEnabledCheck = nullptr;
    QComboBox *m_loggingLevelCombo = nullptr;
};
