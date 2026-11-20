// CUB3 Launcher - installation list filtering and ordering
#pragma once

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

/// Compare text naturally, so numeric runs sort by value (2 before 10).
/// This is locale-independent and behaves consistently on every CI runner.
inline int naturalCompare(const QString& left, const QString& right) {
    qsizetype i = 0;
    qsizetype j = 0;
    while (i < left.size() && j < right.size()) {
        const QChar a = left.at(i);
        const QChar b = right.at(j);
        if (a.isDigit() && b.isDigit()) {
            const qsizetype leftStart = i;
            const qsizetype rightStart = j;
            while (i < left.size() && left.at(i).isDigit())
                ++i;
            while (j < right.size() && right.at(j).isDigit())
                ++j;

            qsizetype leftSignificant = leftStart;
            qsizetype rightSignificant = rightStart;
            while (leftSignificant < i && left.at(leftSignificant).digitValue() == 0)
                ++leftSignificant;
            while (rightSignificant < j && right.at(rightSignificant).digitValue() == 0)
                ++rightSignificant;

            const qsizetype leftDigits = i - leftSignificant;
            const qsizetype rightDigits = j - rightSignificant;
            if (leftDigits != rightDigits)
                return leftDigits < rightDigits ? -1 : 1;
            for (qsizetype offset = 0; offset < leftDigits; ++offset) {
                const int leftDigit = left.at(leftSignificant + offset).digitValue();
                const int rightDigit = right.at(rightSignificant + offset).digitValue();
                if (leftDigit != rightDigit)
                    return leftDigit < rightDigit ? -1 : 1;
            }

            const qsizetype leftRunLength = i - leftStart;
            const qsizetype rightRunLength = j - rightStart;
            if (leftRunLength != rightRunLength)
                return leftRunLength < rightRunLength ? -1 : 1;
            continue;
        }

        const QChar foldedA = a.toCaseFolded();
        const QChar foldedB = b.toCaseFolded();
        if (foldedA != foldedB)
            return foldedA.unicode() < foldedB.unicode() ? -1 : 1;
        ++i;
        ++j;
    }
    if (i == left.size() && j == right.size())
        return 0;
    return i == left.size() ? -1 : 1;
}

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

    std::stable_sort(installations.begin(), installations.end(),
        [sort](const InstallationRecord& left,
               const InstallationRecord& right) {
            switch (sort) {
            case InstallationSort::Name: {
                const int order = naturalCompare(left.name, right.name);
                return order == 0 ? left.id < right.id : order < 0;
            }
            case InstallationSort::Version: {
                const int order = naturalCompare(left.version, right.version);
                if (order != 0)
                    return order > 0;
                const int nameOrder = naturalCompare(left.name, right.name);
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
