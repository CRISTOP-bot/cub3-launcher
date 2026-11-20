#include <QtTest>

#include "gitlab/GitLabManager.h"

class GitLabManagerTests final : public QObject {
    Q_OBJECT
private slots:
    void acceptsHttpsGitLabInstances() {
        QVERIFY(cub3::gitlab::GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://gitlab.com"))));
        QVERIFY(cub3::gitlab::GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://gitlab.example.org/"))));
        QVERIFY(cub3::gitlab::GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://git.example.org/gitlab"))));
    }
    void rejectsUnsafeInstanceUrls() {
        using cub3::gitlab::GitLabManager;
        QVERIFY(!GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("http://gitlab.com"))));
        QVERIFY(!GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://user:pass@gitlab.com"))));
        QVERIFY(!GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://gitlab.com/?token=secret"))));
        QVERIFY(!GitLabManager::isValidInstanceUrl(QUrl(QStringLiteral("https://gitlab.com/../evil"))));
        QVERIFY(!GitLabManager::isValidInstanceUrl(QUrl()));
    }
    void acceptsOnlyNumericProjectIds() {
        using cub3::gitlab::GitLabManager;
        QVERIFY(GitLabManager::isValidProjectId(QStringLiteral("42")));
        QVERIFY(GitLabManager::isValidProjectId(QStringLiteral("1000000000000")));
        QVERIFY(!GitLabManager::isValidProjectId(QString{}));
        QVERIFY(!GitLabManager::isValidProjectId(QStringLiteral("group/project")));
        QVERIFY(!GitLabManager::isValidProjectId(QStringLiteral("42?private_token=secret")));
        QVERIFY(!GitLabManager::isValidProjectId(QStringLiteral("-1")));
    }
};

QTEST_APPLESS_MAIN(GitLabManagerTests)
#include "GitLabManagerTests.moc"
