// CUB3 Launcher - resource pack option editing tests
#include "application/CppEditionPack.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QtTest>

using cub3::application::CppEditionPack;

class CppEditionPackTests final : public QObject {
    Q_OBJECT
private slots:
    void addsPackWithoutDroppingOtherOptions() {
        QByteArray options = QByteArrayLiteral("sound:0.8\nresourcePacks:[\"vanilla\",\"file/OtherPack.zip\"]\nlang:en_us\n");
        QString error;
        QVERIFY2(CppEditionPack::updateSelection(options, true, &error), qPrintable(error));
        QVERIFY(options.contains("sound:0.8"));
        QVERIFY(options.contains("lang:en_us"));
        QVERIFY(options.contains("\"vanilla\""));
        QVERIFY(options.contains("\"file/OtherPack.zip\""));
        QVERIFY(options.contains(QString::fromLatin1(CppEditionPack::kResourcePackId).toUtf8()));
        const QByteArray line = options.split('\n').at(1);
        const QJsonArray array = QJsonDocument::fromJson(line.mid(QByteArrayLiteral("resourcePacks:").size())).array();
        QCOMPARE(array.size(), 3);
    }

    void doesNotDuplicateExistingPackAndCanRemoveIt() {
        QByteArray options = QByteArrayLiteral("resourcePacks:[\"vanilla\",\"file/CUB3-Cpp-Edition.zip\",\"file/CUB3-Cpp-Edition.zip\"]\n");
        QString error;
        QVERIFY2(CppEditionPack::updateSelection(options, true, &error), qPrintable(error));
        QCOMPARE(QJsonDocument::fromJson(options.trimmed().mid(14)).array().size(), 2);
        QVERIFY2(CppEditionPack::updateSelection(options, false, &error), qPrintable(error));
        const QJsonArray packs = QJsonDocument::fromJson(options.trimmed().mid(14)).array();
        QCOMPARE(packs.size(), 1);
        QCOMPARE(packs.first().toString(), QStringLiteral("vanilla"));
    }

    void malformedOptionsAreLeftUntouched() {
        QByteArray options = QByteArrayLiteral("resourcePacks:not-json\nvideo:1\n");
        const QByteArray original = options;
        QString error;
        QVERIFY(!CppEditionPack::updateSelection(options, true, &error));
        QCOMPARE(options, original);
        QVERIFY(!error.isEmpty());
    }


    void preservesCrLfAndRejectsDuplicateOptionsKeys() {
        QByteArray options = QByteArrayLiteral("resourcePacks:[\"vanilla\"]\r\nvideo:1\r\n");
        QString error;
        QVERIFY2(CppEditionPack::updateSelection(options, true, &error), qPrintable(error));
        QVERIFY(options.contains("\r\n"));
        QVERIFY(!options.contains("\nvideo:1\n"));

        options = QByteArrayLiteral("resourcePacks:[\"vanilla\"]\nresourcePacks:[\"other\"]\n");
        const QByteArray original = options;
        QVERIFY(!CppEditionPack::updateSelection(options, true, &error));
        QCOMPARE(options, original);
    }

    void canCreatePackSelectionForFreshInstance() {
        QByteArray options;
        QString error;
        QVERIFY2(CppEditionPack::updateSelection(options, true, &error), qPrintable(error));
        QCOMPARE(options, QByteArrayLiteral("resourcePacks:[\"file/CUB3-Cpp-Edition.zip\"]\n"));
    }
};

QTEST_GUILESS_MAIN(CppEditionPackTests)
#include "CppEditionPackTests.moc"
