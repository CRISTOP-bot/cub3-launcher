#include <QtTest>

#include "network/HttpClient.h"

class NetworkSecurityTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsOnlyCredentialFreeHttps() {
        QVERIFY(cub3::network::HttpClient::isSecureUrl(
            QUrl(QStringLiteral("https://piston-meta.mojang.com/manifest.json"))));
        QVERIFY(!cub3::network::HttpClient::isSecureUrl(
            QUrl(QStringLiteral("http://example.com/file"))));
        QVERIFY(!cub3::network::HttpClient::isSecureUrl(
            QUrl(QStringLiteral("https://user:secret@example.com/file"))));
        QVERIFY(!cub3::network::HttpClient::isSecureUrl(
            QUrl(QStringLiteral("https:///missing-host"))));
        QVERIFY(!cub3::network::HttpClient::isSecureUrl(QUrl()));
    }

};

QTEST_APPLESS_MAIN(NetworkSecurityTests)
#include "NetworkSecurityTests.moc"
