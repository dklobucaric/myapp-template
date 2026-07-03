#pragma once

#include <QDialog>
#include <QStringList>

#include "core/ConfigManager.h"

class QListWidget;
class QStackedWidget;

class SettingsDialog final : public QDialog
{
public:
    explicit SettingsDialog(const AppConfig &config, QWidget *parent = nullptr);

private:
    QWidget *createDetailsPage(
        const QString &title,
        const QStringList &lines
    ) const;

    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
};
