// CUB3 Launcher - Authentication session model tests
#include "auth/Account.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using cub3::auth::Account;
using cub3::auth::Provider;

class AuthSessionTests final : public QObject {
    Q_OBJECT

private slots:
    void roundTripPreservesProviderAndMinecraftProfile() {
        Account original;
        original.id = 42;
        original.provider = Provider::Elyby;
        original.username = QStringLiteral("Player");
        original.uuid = QStringLiteral("a1b2c3d4");
        original.accessToken = QStringLiteral("access-token-test-value");
        original.refreshToken = QStringLiteral("refresh-token-test-value");
        original.expiresAt = QDateTime::currentDateTime().addSecs(3600);
        original.skinUrl = QStringLiteral("https://textures.example/skin.png");

        const Account restored = Account::fromJson(original.toJson());
        QCOMPARE(restored.id, original.id);
        QVERIFY(restored.provider == Provider::Elyby);
        QCOMPARE(restored.username, original.username);
        QCOMPARE(restored.uuid, original.uuid);
        QCOMPARE(restored.accessToken, original.accessToken);
        QCOMPARE(restored.refreshToken, original.refreshToken);
        QCOMPARE(restored.expiresAt.toSecsSinceEpoch(), original.expiresAt.toSecsSinceEpoch());
        QCOMPARE(restored.skinUrl, original.skinUrl);
    }

    void microsoftProviderSurvivesRoundTrip() {
        Account original;
        original.provider = Provider::Microsoft;
        original.username = QStringLiteral("Alex");
        original.uuid = QStringLiteral("minecraft-uuid");
        original.accessToken = QStringLiteral("minecraft-access-token");
        original.refreshToken = QStringLiteral("oauth-refresh-token");
        original.expiresAt = QDateTime::currentDateTime().addSecs(900);
        original.skinUrl = QStringLiteral("https://textures.example/alex.png");

        const Account restored = Account::fromJson(original.toJson());
        QVERIFY(restored.provider == Provider::Microsoft);
        QCOMPARE(restored.providerString(), QStringLiteral("microsoft"));
        QCOMPARE(restored.username, original.username);
        QCOMPARE(restored.uuid, original.uuid);
        QCOMPARE(restored.skinUrl, original.skinUrl);
    }

    void validityAndRefreshThresholdAreConsistent() {
        Account account;
        account.username = QStringLiteral("Player");
        account.accessToken = QStringLiteral("token");
        account.expiresAt = QDateTime::currentDateTime().addSecs(120);
        QVERIFY(account.isValid());
        QVERIFY(!account.needsRefresh());

        account.expiresAt = QDateTime::currentDateTime().addSecs(30);
        QVERIFY(account.isValid());
        QVERIFY(account.needsRefresh());

        account.expiresAt = QDateTime::currentDateTime().addSecs(-1);
        QVERIFY(!account.isValid());
        QVERIFY(account.needsRefresh());
    }

};

QTEST_MAIN(AuthSessionTests)
#include "AuthSessionTests.moc"
