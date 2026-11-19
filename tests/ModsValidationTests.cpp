#include <QtTest>

#include "ui/pages/ModsValidation.h"

class ModsValidationTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsReasonableMinecraftVersionIdentifiers() {
        QVERIFY(cub3::ui::pages::modsValidation::isMinecraftVersion(
            QStringLiteral("1.21.10")));
        QVERIFY(cub3::ui::pages::modsValidation::isMinecraftVersion(
            QStringLiteral(" 24w14a ")));
        QVERIFY(!cub3::ui::pages::modsValidation::isMinecraftVersion(QString{}));
        QVERIFY(!cub3::ui::pages::modsValidation::isMinecraftVersion(
            QStringLiteral("../../mods")));
        QVERIFY(!cub3::ui::pages::modsValidation::isMinecraftVersion(
            QStringLiteral("1.21/../../outside")));
    }

    void acceptsModrinthProjectIdsOnly() {
        QVERIFY(cub3::ui::pages::modsValidation::isModrinthProjectId(
            QStringLiteral("A1b2C3d4")));
        QVERIFY(!cub3::ui::pages::modsValidation::isModrinthProjectId(
            QStringLiteral("../escape")));
        QVERIFY(!cub3::ui::pages::modsValidation::isModrinthProjectId(
            QStringLiteral("short")));
    }

    void acceptsJarBasenameAndRejectsPathTraversal() {
        QCOMPARE(cub3::ui::pages::modsValidation::safeJarFileName(
                     QStringLiteral("Cool-Mod_1.2.jar")),
                 QStringLiteral("Cool-Mod_1.2.jar"));
        QCOMPARE(cub3::ui::pages::modsValidation::safeJarFileName(
                     QStringLiteral("../../outside.jar")),
                 QStringLiteral("outside.jar"));
        QVERIFY(cub3::ui::pages::modsValidation::safeJarFileName(
                    QStringLiteral("not-a-jar.zip")).isEmpty());
    }
};

QTEST_APPLESS_MAIN(ModsValidationTests)
#include "ModsValidationTests.moc"
