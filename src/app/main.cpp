#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDebug>

#include "core/ConfigManager.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Bootstrap metadata establishes the default AppData location before the
    // product profile is loaded. Future template clones update these values.
    QCoreApplication::setApplicationName(QStringLiteral("MyAppTemplate"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("MyAppTemplate"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MYAPP_TEMPLATE_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("DD-Lab"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("dd-lab.hr"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Reusable desktop application template")
    );
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption profileOption(
        {QStringLiteral("p"), QStringLiteral("profile")},
        QStringLiteral("Use the development, staging or production environment profile."),
        QStringLiteral("name")
    );
    parser.addOption(profileOption);
    parser.process(app);

    const ConfigLoadResult loadResult = ConfigManager::load(
        parser.value(profileOption)
    );

    for (const QString &warning : loadResult.warnings) {
        qWarning().noquote() << "Configuration warning:" << warning;
    }

    if (!loadResult.isUsable()) {
        qCritical() << "The application profile could not be loaded.";
        return 1;
    }

    const AppConfig &config = loadResult.config;
    QGuiApplication::setApplicationDisplayName(config.appName);
    QCoreApplication::setApplicationVersion(config.version);
    QCoreApplication::setOrganizationName(config.vendor);

    MainWindow window(config, loadResult.warnings);
    window.show();

    return app.exec();
}
