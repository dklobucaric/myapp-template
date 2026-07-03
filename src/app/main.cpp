#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("MyAppTemplate"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("MyAppTemplate"));
    QCoreApplication::setApplicationVersion(QStringLiteral(MYAPP_TEMPLATE_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("DD-Lab"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("dd-lab.hr"));

    MainWindow window;
    window.show();

    return app.exec();
}
