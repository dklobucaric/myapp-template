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
    void clearingUserOverridesRestoresEnvironmentValues();
    void previewConfigUsesBaselineWhenOverridesChange();
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


void ConfigManagerTest::clearingUserOverridesRestoresEnvironmentValues()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QJsonObject overrides{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("services"), QJsonObject{
            {QStringLiteral("cdnBaseUrl"), QStringLiteral("https://override.example.invalid/cdn")}
        }}
    };

    QString errorMessage;
    QVERIFY(ConfigManager::saveUserOverrides(
        overrides,
        temporaryDirectory.path(),
        &errorMessage
    ));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    const ConfigLoadResult overridden = ConfigManager::load(
        QStringLiteral("staging"),
        temporaryDirectory.path()
    );
    QCOMPARE(
        overridden.config.cdnBaseUrl,
        QStringLiteral("https://override.example.invalid/cdn")
    );

    QVERIFY(ConfigManager::saveUserOverrides(
        ConfigManager::emptyUserOverrides(),
        temporaryDirectory.path(),
        &errorMessage
    ));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    const ConfigLoadResult restored = ConfigManager::load(
        QStringLiteral("staging"),
        temporaryDirectory.path()
    );
    QCOMPARE(
        restored.config.cdnBaseUrl,
        QStringLiteral("https://staging-cdn.dd-lab.hr/products/myapp-template")
    );
    QCOMPARE(restored.config.updateChannel, QStringLiteral("beta"));
}

void ConfigManagerTest::previewConfigUsesBaselineWhenOverridesChange()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ConfigLoadResult baseline = ConfigManager::load(
        QStringLiteral("production"),
        temporaryDirectory.path()
    );
    QVERIFY2(baseline.isUsable(), qPrintable(baseline.warnings.join('\n')));

    QJsonObject overrides = ConfigManager::emptyUserOverrides();
    overrides.insert(QStringLiteral("appearance"), QJsonObject{
        {QStringLiteral("theme"), QStringLiteral("dark")}
    });
    overrides.insert(QStringLiteral("layout"), QJsonObject{
        {QStringLiteral("showStatusBar"), false}
    });

    const AppConfig preview = ConfigManager::configWithUserOverrides(
        baseline.config,
        overrides
    );
    QCOMPARE(preview.theme, QStringLiteral("dark"));
    QVERIFY(!preview.showStatusBar);
    QCOMPARE(
        preview.cdnBaseUrl,
        QStringLiteral("https://cdn.dd-lab.hr/products/myapp-template")
    );

    const AppConfig resetPreview = ConfigManager::configWithUserOverrides(
        baseline.config,
        ConfigManager::emptyUserOverrides()
    );
    QCOMPARE(resetPreview.theme, QStringLiteral("system"));
    QVERIFY(resetPreview.showStatusBar);
    QCOMPARE(
        resetPreview.cdnBaseUrl,
        QStringLiteral("https://cdn.dd-lab.hr/products/myapp-template")
    );
}

QTEST_GUILESS_MAIN(ConfigManagerTest)

#include "ConfigManagerTest.moc"
