#pragma once

#include <QMainWindow>

class QLabel;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createMenuBar();
    void createStatusBar();

    void showSettingsPlaceholder();
    void showUpdatePlaceholder();
    void showLicensePlaceholder();
    void showAboutDialog();

    QLabel *m_licenseIndicator = nullptr;
    QLabel *m_updateIndicator = nullptr;
    QLabel *m_versionIndicator = nullptr;
};
