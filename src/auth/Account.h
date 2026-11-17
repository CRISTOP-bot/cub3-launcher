// CUB3 Launcher - Account model
#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>

namespace cub3::auth {

/// Authentication provider type.
enum class Provider {
    Microsoft,
    Elyby
};

/// An authenticated account.
struct Account {
    int id{-1};
    Provider provider{Provider::Microsoft};
    QString username;
    QString uuid;
    QString accessToken;
    QString refreshToken;
    QDateTime expiresAt;
    QString skinUrl;

    [[nodiscard]] bool isValid() const {
        return !username.isEmpty() && !accessToken.isEmpty()
               && (!expiresAt.isValid() || expiresAt > QDateTime::currentDateTime());
    }

    [[nodiscard]] bool needsRefresh() const {
        return expiresAt.isValid() && expiresAt <= QDateTime::currentDateTime().addSecs(60);
    }

    [[nodiscard]] QString providerString() const {
        return provider == Provider::Microsoft ? "microsoft" : "elyby";
    }

    [[nodiscard]] QJsonObject toJson() const {
        return {
            {"id", id},
            {"provider", providerString()},
            {"username", username},
            {"uuid", uuid},
            {"accessToken", accessToken},
            {"refreshToken", refreshToken},
            {"expiresAt", expiresAt.toSecsSinceEpoch()},
            {"skinUrl", skinUrl}
        };
    }

    static Account fromJson(const QJsonObject& obj) {
        Account a;
        a.id = obj.value("id").toInt();
        a.provider = obj.value("provider").toString() == "microsoft"
                     ? Provider::Microsoft : Provider::Elyby;
        a.username = obj.value("username").toString();
        a.uuid = obj.value("uuid").toString();
        a.accessToken = obj.value("accessToken").toString();
        a.refreshToken = obj.value("refreshToken").toString();
        a.expiresAt = QDateTime::fromSecsSinceEpoch(obj.value("expiresAt").toVariant().toLongLong());
        a.skinUrl = obj.value("skinUrl").toString();
        return a;
    }
};

} // namespace cub3::auth
