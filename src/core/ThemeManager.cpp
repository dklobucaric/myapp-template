#include "core/ThemeManager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QColor>
#include <QFont>
#include <QPalette>

#include <algorithm>

namespace
{
bool s_defaultsCaptured = false;
QPalette s_systemPalette;
QFont s_systemFont;
QString s_systemStyleSheet;

QColor readableTextColor(const QColor &background)
{
    const int luminance =
        (background.red() * 299 + background.green() * 587 + background.blue() * 114) / 1000;
    return luminance >= 150 ? QColor(Qt::black) : QColor(Qt::white);
}

QPalette lightPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(250, 250, 250));
    palette.setColor(QPalette::WindowText, QColor(32, 32, 32));
    palette.setColor(QPalette::Base, QColor(Qt::white));
    palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    palette.setColor(QPalette::ToolTipBase, QColor(Qt::white));
    palette.setColor(QPalette::ToolTipText, QColor(32, 32, 32));
    palette.setColor(QPalette::Text, QColor(32, 32, 32));
    palette.setColor(QPalette::Button, QColor(245, 245, 245));
    palette.setColor(QPalette::ButtonText, QColor(32, 32, 32));
    palette.setColor(QPalette::BrightText, QColor(Qt::red));
    return palette;
}

QPalette darkPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(45, 45, 48));
    palette.setColor(QPalette::WindowText, QColor(238, 238, 238));
    palette.setColor(QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, QColor(45, 45, 48));
    palette.setColor(QPalette::ToolTipText, QColor(238, 238, 238));
    palette.setColor(QPalette::Text, QColor(238, 238, 238));
    palette.setColor(QPalette::Button, QColor(55, 55, 58));
    palette.setColor(QPalette::ButtonText, QColor(238, 238, 238));
    palette.setColor(QPalette::BrightText, QColor(255, 90, 90));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(145, 145, 145));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(145, 145, 145));
    return palette;
}

void applyAccentColor(QPalette &palette, const QString &accentColor)
{
    const QColor accent(accentColor);
    if (!accent.isValid()) {
        return;
    }

    const QColor textColor = readableTextColor(accent);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, textColor);
    palette.setColor(QPalette::Active, QPalette::Highlight, accent);
    palette.setColor(QPalette::Active, QPalette::HighlightedText, textColor);
    palette.setColor(QPalette::Inactive, QPalette::Highlight, accent);
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, textColor);
}
} // namespace

void ThemeManager::captureApplicationDefaults()
{
    if (s_defaultsCaptured) {
        return;
    }

    auto *application = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (application == nullptr) {
        return;
    }

    s_systemPalette = application->palette();
    s_systemFont = application->font();
    s_systemStyleSheet = application->styleSheet();
    s_defaultsCaptured = true;
}

void ThemeManager::apply(const AppConfig &config)
{
    captureApplicationDefaults();

    auto *application = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (application == nullptr || !s_defaultsCaptured) {
        return;
    }

    const QString theme = config.theme.trimmed().toLower();
    QPalette palette = s_systemPalette;

    if (theme == QStringLiteral("light")) {
        palette = lightPalette();
    } else if (theme == QStringLiteral("dark")) {
        palette = darkPalette();
    }

    applyAccentColor(palette, config.accentColor);
    application->setStyleSheet(s_systemStyleSheet);
    application->setPalette(palette);

    QFont font = s_systemFont;
    const double scale = std::clamp(config.fontScale, 0.75, 2.0);
    const qreal basePointSize = font.pointSizeF() > 0.0 ? font.pointSizeF() : 10.0;
    font.setPointSizeF(std::max<qreal>(6.0, basePointSize * scale));
    application->setFont(font);
}
