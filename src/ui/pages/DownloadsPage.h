// CUB3 Launcher - Downloads page
#pragma once

#include <QWidget>
#include <QListWidget>

class QLabel;

namespace cub3::ui::pages {

/// Shows active downloads with speed, progress, status, and error info.
/// Items are updated incrementally — no full list rebuild on each progress tick.
class DownloadsPage : public QWidget {
    Q_OBJECT

public:
    explicit DownloadsPage(QWidget* parent = nullptr);

    /// Add or update a download entry.
    /// Returns the item index.
    int upsertDownload(const QString& id, const QString& name,
                       int progress, qint64 speed, const QString& status);

    /// Remove a completed/cancelled download.
    void removeDownload(const QString& id);

    /// Update the global speed indicator.
    void setGlobalSpeed(qint64 bytesPerSec);

private:
    void setupUi();

    QListWidget* m_list{nullptr};
    QLabel* m_speedLabel{nullptr};
    QLabel* m_countLabel{nullptr};
};

} // namespace cub3::ui::pages
