#pragma once

#include <QStringList>
#include <QWidget>

class QGroupBox;

class TemplateDashboard final : public QWidget
{
public:
    explicit TemplateDashboard(QWidget *parent = nullptr);

private:
    QGroupBox *createSection(
        const QString &title,
        const QStringList &lines
    ) const;
};
