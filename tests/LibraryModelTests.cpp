#include "ui/pages/LibraryModel.h"

#include <QtTest/QtTest>

using cub3::ui::pages::InstallationRecord;
using cub3::ui::pages::InstallationSort;
using cub3::ui::pages::filterAndSortInstallations;

class LibraryModelTests final : public QObject {
    Q_OBJECT

private slots:
    void filtersAcrossNameVersionAndLoader() {
        const QList<InstallationRecord> installations{
            {1, QStringLiteral("Survival"), QStringLiteral("1.20.4"), QStringLiteral("fabric"), {}, 10},
            {2, QStringLiteral("Creative"), QStringLiteral("1.21.1"), QStringLiteral("vanilla"), {}, 20},
            {3, QStringLiteral("Modded"), QStringLiteral("1.19.2"), QStringLiteral("forge"), {}, 30},
        };

        QCOMPARE(filterAndSortInstallations(installations, QStringLiteral("surv"),
                                            InstallationSort::RecentlyAdded).size(), 1);
        QCOMPARE(filterAndSortInstallations(installations, QStringLiteral("1.21"),
                                            InstallationSort::RecentlyAdded).first().id, qint64(2));
        QCOMPARE(filterAndSortInstallations(installations, QStringLiteral("FORGE"),
                                            InstallationSort::RecentlyAdded).first().id, qint64(3));
        QCOMPARE(filterAndSortInstallations(installations, QStringLiteral("  "),
                                            InstallationSort::RecentlyAdded).size(), 3);
    }

    void comparesNumericRunsByValueAndKeepsLeadingZeroesDeterministic() {
        using cub3::ui::pages::naturalCompare;
        QVERIFY(naturalCompare(QStringLiteral("World 2"), QStringLiteral("World 10")) < 0);
        QVERIFY(naturalCompare(QStringLiteral("1.10"), QStringLiteral("1.9")) > 0);
        QVERIFY(naturalCompare(QStringLiteral("Pack 2"), QStringLiteral("Pack 02")) < 0);
        QCOMPARE(naturalCompare(QStringLiteral("SAME"), QStringLiteral("same")), 0);
    }

    void sortsByNameNaturallyAndDeterministically() {
        const QList<InstallationRecord> installations{
            {1, QStringLiteral("Instance 10"), QStringLiteral("1.20"), {}, {}, 2},
            {2, QStringLiteral("instance 2"), QStringLiteral("1.20"), {}, {}, 1},
            {3, QStringLiteral("Alpha"), QStringLiteral("1.20"), {}, {}, 1},
        };
        const auto sorted = filterAndSortInstallations(installations, {}, InstallationSort::Name);
        QCOMPARE(sorted.at(0).id, qint64(3));
        QCOMPARE(sorted.at(1).id, qint64(2));
        QCOMPARE(sorted.at(2).id, qint64(1));
    }

    void sortsVersionsAndRecencyDescending() {
        const QList<InstallationRecord> installations{
            {1, QStringLiteral("Old"), QStringLiteral("1.9"), {}, {}, 1},
            {2, QStringLiteral("Newer"), QStringLiteral("1.10"), {}, {}, 3},
            {3, QStringLiteral("Newest"), QStringLiteral("1.21"), {}, {}, 2},
        };
        const auto versions = filterAndSortInstallations(installations, {}, InstallationSort::Version);
        QCOMPARE(versions.at(0).id, qint64(3));
        QCOMPARE(versions.at(1).id, qint64(2));
        QCOMPARE(versions.at(2).id, qint64(1));
        const auto recent = filterAndSortInstallations(installations, {}, InstallationSort::RecentlyAdded);
        QCOMPARE(recent.at(0).id, qint64(2));
        QCOMPARE(recent.at(1).id, qint64(3));
        QCOMPARE(recent.at(2).id, qint64(1));
    }
};

QTEST_GUILESS_MAIN(LibraryModelTests)
#include "LibraryModelTests.moc"
