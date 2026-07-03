#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

#include "core/ConfigManager.h"

class ConfigManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void builtInResourcesAreAvailable();
    void createsInitialUserConfig();
    void environmentProfileOverridesDefaultEndpoints();
    void userConfigOverridesEnvironmentProfile();
    void malformedUserConfigFallsBackSafely();
};

void ConfigManagerTest::builtInResourcesAreAvailable()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ConfigLoadResult result = ConfigManager::load(
        QStringLiteral("development"),
        temporaryDirectory.path()
    );
    QVERIFY2(result.isUsable(), qPrintable(result.warnings.join('\n')));

    QFile appProfile(QStringLiteral(":/defaults/app-profile.json"));
    QVERIFY2(appProfile.exists(), "Built-in app profile resource is unavailable.");
    QVERIFY(appProfile.open(QIODevice::ReadOnly));

    QFile developmentProfile(QStringLiteral(":/profiles/development.json"));
    QVERIFY2(developmentProfile.exists(), "Built-in development profile resource is unavailable.");
    QVERIFY(developmentProfile.open(QIODevice::ReadOnly));
}

void ConfigManagerTest::createsInitialUserConfig()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ConfigLoadResult result = ConfigManager::load(
        QStringLiteral("development"),
        temporaryDirectory.path()
    );

    QVERIFY(result.isUsable());
    QVERIFY(result.config.userConfigCreated);
    QCOMPARE(result.config.environment, QStringLiteral("development"));
    QCOMPARE(result.config.appName, QStringLiteral("MyAppTemplate"));
    QVERIFY(QFile::exists(result.config.userConfigPath));

    QFile file(result.config.userConfigPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("schemaVersion")).toInt(), 1);
    QVERIFY(document.object().value(QStringLiteral("appearance")).isObject());
    QVERIFY(document.object().value(QStringLiteral("services")).isObject());
}

void ConfigManagerTest::environmentProfileOverridesDefaultEndpoints()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ConfigLoadResult result = ConfigManager::load(
        QStringLiteral("staging"),
        temporaryDirectory.path()
    );

    QVERIFY(result.isUsable());
    QCOMPARE(result.config.environment, QStringLiteral("staging"));
    QCOMPARE(result.config.updateChannel, QStringLiteral("beta"));
    QCOMPARE(
        result.config.cdnBaseUrl,
        QStringLiteral("https://staging-cdn.dd-lab.hr/products/myapp-template")
    );
    QCOMPARE(
        result.config.licenseApiBaseUrl,
        QStringLiteral("https://staging-licence.dd-lab.hr/api/v1")
    );
}

void ConfigManagerTest::userConfigOverridesEnvironmentProfile()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QJsonObject userOverrides{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("layout"), QJsonObject{
            {QStringLiteral("windowWidth"), 1440},
            {QStringLiteral("windowHeight"), 900}
        }},
        {QStringLiteral("updates"), QJsonObject{
            {QStringLiteral("channel"), QStringLiteral("beta")}
        }},
        {QStringLiteral("services"), QJsonObject{
            {QStringLiteral("cdnBaseUrl"), QStringLiteral("https://override.example.invalid/cdn")}
        }}
    };

    QString errorMessage;
    QVERIFY(ConfigManager::saveUserOverrides(
        userOverrides,
        temporaryDirectory.path(),
        &errorMessage
    ));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    const ConfigLoadResult result = ConfigManager::load(
        QStringLiteral("staging"),
        temporaryDirectory.path()
    );

    QCOMPARE(result.config.windowWidth, 1440);
    QCOMPARE(result.config.windowHeight, 900);
    QCOMPARE(result.config.updateChannel, QStringLiteral("beta"));
    QCOMPARE(
        result.config.cdnBaseUrl,
        QStringLiteral("https://override.example.invalid/cdn")
    );
    QCOMPARE(
        result.config.licenseApiBaseUrl,
        QStringLiteral("https://staging-licence.dd-lab.hr/api/v1")
    );
}

void ConfigManagerTest::malformedUserConfigFallsBackSafely()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString configPath = ConfigManager::userConfigPath(temporaryDirectory.path());
    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("{ invalid json");
    file.close();

    const ConfigLoadResult result = ConfigManager::load(
        QStringLiteral("development"),
        temporaryDirectory.path()
    );

    QVERIFY(result.isUsable());
    QVERIFY(!result.warnings.isEmpty());
    QCOMPARE(result.config.windowWidth, 1200);
    QCOMPARE(
        result.config.cdnBaseUrl,
        QStringLiteral("http://localhost:8088/products/myapp-template")
    );
}

QTEST_GUILESS_MAIN(ConfigManagerTest)

#include "ConfigManagerTest.moc"
