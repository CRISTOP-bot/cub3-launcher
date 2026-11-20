// CUB3 - Discord IPC protocol framing and configuration tests
#include "discord/DiscordPresence.h"

#include <QtEndian>
#include <QtTest>

class DiscordPresenceTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsOnlyPublicDiscordApplicationIds() {
        QVERIFY(cub3::discord::DiscordPresence::isValidApplicationId(
            QStringLiteral("12345678901234567")));
        QVERIFY(cub3::discord::DiscordPresence::isValidApplicationId(
            QStringLiteral("12345678901234567890")));
        QVERIFY(!cub3::discord::DiscordPresence::isValidApplicationId(QString{}));
        QVERIFY(!cub3::discord::DiscordPresence::isValidApplicationId(
            QStringLiteral("abc12345678901234")));
        QVERIFY(!cub3::discord::DiscordPresence::isValidApplicationId(
            QStringLiteral(" 12345678901234567 ")));
        QVERIFY(!cub3::discord::DiscordPresence::isValidApplicationId(
            QStringLiteral("123456789012345678901")));
    }

    void framesUseDiscordLittleEndianHeaderAndPayload() {
        const QByteArray payload = QByteArrayLiteral("{\"v\":1}");
        const QByteArray frame = cub3::discord::DiscordPresence::encodeFrame(1, payload);
        QCOMPARE(frame.size(), 8 + payload.size());
        const auto* bytes = reinterpret_cast<const uchar*>(frame.constData());
        QCOMPARE(qFromLittleEndian<quint32>(bytes), quint32(1));
        QCOMPARE(qFromLittleEndian<quint32>(bytes + 4),
                 static_cast<quint32>(payload.size()));
        QCOMPARE(frame.mid(8), payload);
    }
};

QTEST_GUILESS_MAIN(DiscordPresenceTests)
#include "DiscordPresenceTests.moc"
