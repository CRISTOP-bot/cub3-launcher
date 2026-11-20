// CUB3 Launcher - installation list filtering and ordering
#pragma once

#include <QCollator>
#include <QList>
#include <QString>

#include <algorithm>

namespace cub3::ui::pages {

struct InstallationRecord {
    qint64 id{0};
    QString name;
    QString version;
    QString loader;
    QString directory;
    qint64 createdAt{0};
};

enum class InstallationSort {
    RecentlyAdded,
    Name,
    Version,
};

inline QList<InstallationRecord> filterAndSortInstallations(
    QList<InstallationRecord> installations,
    const QString& searchText,
    InstallationSort sort)
{
    const QString needle = searchText.trimmed();
    if (!needle.isEmpty()) {
        installations.erase(
            std::remove_if(installations.begin(), installations.end(),
                [&needle](const InstallationRecord& installation) {
                    return !installation.name.contains(needle, Qt::CaseInsensitive)
                        && !installation.version.contains(needle, Qt::CaseInsensitive)
                        && !installation.loader.contains(needle, Qt::CaseInsensitive);
                }),
            installations.end());
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(installations.begin(), installations.end(),
        [sort, &collator](const InstallationRecord& left,
                          const InstallationRecord& right) {
            switch (sort) {
            case InstallationSort::Name: {
                const int order = collator.compare(left.name, right.name);
                return order == 0 ? left.id < right.id : order < 0;
            }
            case InstallationSort::Version: {
                const int order = collator.compare(left.version, right.version);
                if (order != 0)
                    return order > 0;
                const int nameOrder = collator.compare(left.name, right.name);
                return nameOrder == 0 ? left.id < right.id : nameOrder < 0;
            }
            case InstallationSort::RecentlyAdded:
            default:
                return left.createdAt == right.createdAt
                    ? left.id > right.id : left.createdAt > right.createdAt;
            }
        });
    return installations;
}

} // namespace cub3::ui::pages
