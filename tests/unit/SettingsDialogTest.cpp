#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest>

#include "core/ConfigManager.h"
#include "settings/SettingsDialog.h"

class SettingsDialogTest final : public QObject
{
    Q_OBJECT

private slots:
    void loggingLevelPersistsAndDoesNotSnapBack();
};

void SettingsDialogTest::loggingLevelPersistsAndDoesNotSnapBack()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QJsonObject overrides = ConfigManager::emptyUserOverrides();
    QJsonObject diagnostics = overrides.value(QStringLiteral("diagnostics")).toObject();
    diagnostics.insert(QStringLiteral("loggingLevel"), QStringLiteral("error"));
    overrides.insert(QStringLiteral("diagnostics"), diagnostics);

    QString errorMessage;
    QVERIFY2(
        ConfigManager::saveUserOverrides(overrides, directory.path(), &errorMessage),
        qPrintable(errorMessage)
    );

    const ConfigLoadResult initialLoad = ConfigManager::load(QStringLiteral("development"), directory.path());
    QVERIFY(initialLoad.isUsable());
    QCOMPARE(initialLoad.config.loggingLevel, QStringLiteral("error"));

    SettingsDialog dialog(initialLoad.config);
    auto *levelCombo = dialog.findChild<QComboBox *>(QStringLiteral("loggingLevelCombo"));
    QVERIFY(levelCombo != nullptr);
    QCOMPARE(levelCombo->currentText(), QStringLiteral("error"));

    const int infoIndex = levelCombo->findText(QStringLiteral("info"), Qt::MatchFixedString);
    QVERIFY(infoIndex >= 0);
    levelCombo->setCurrentIndex(infoIndex);

    auto *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons != nullptr);
    auto *applyButton = buttons->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton != nullptr);
    applyButton->click();

    QCOMPARE(levelCombo->currentText(), QStringLiteral("info"));

    const ConfigLoadResult savedLoad = ConfigManager::load(QStringLiteral("development"), directory.path());
    QVERIFY(savedLoad.isUsable());
    QCOMPARE(savedLoad.config.loggingLevel, QStringLiteral("info"));
}

QTEST_MAIN(SettingsDialogTest)

#include "SettingsDialogTest.moc"
