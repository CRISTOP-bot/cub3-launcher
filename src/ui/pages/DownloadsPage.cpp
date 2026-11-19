// CUB3 Launcher - Downloads page implementation
#include "DownloadsPage.h"
#include "styles/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

namespace cub3::ui::pages {

namespace {
/// Format a byte rate into a human-readable string.
QString formatSpeed(qint64 bps) {
    if (bps < 1024) return QStringLiteral("%1 B/s").arg(bps);
    if (bps < 1024 * 1024) return QStringLiteral("%1 KB/s").arg(bps / 1024);
    return QStringLiteral("%1 MB/s").arg(bps / (1024 * 1024));
}

QString formatDownloadCount(int count) {
    return count == 1
        ? QStringLiteral("1 descarga activa")
        : QStringLiteral("%1 descargas activas").arg(count);
}
} // namespace

DownloadsPage::DownloadsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void DownloadsPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge,
                                theme::sizes::kSpacingLarge);
    layout->setSpacing(theme::sizes::kSpacingMedium);

    auto* titleLabel = new QLabel(QStringLiteral("Descargas"), this);
    titleLabel->setObjectName(QStringLiteral("PageTitle"));
    layout->addWidget(titleLabel);

    // Speed + count row
    auto* infoRow = new QHBoxLayout();
    infoRow->setSpacing(theme::sizes::kSpacingMedium);

    m_countLabel = new QLabel(QStringLiteral("Sin descargas activas"), this);
    m_countLabel->setObjectName(QStringLiteral("PageSubtitle"));
    infoRow->addWidget(m_countLabel);
    infoRow->addStretch();

    m_speedLabel = new QLabel(QStringLiteral(""), this);
    m_speedLabel->setObjectName(QStringLiteral("Secondary"));
    infoRow->addWidget(m_speedLabel);

    layout->addLayout(infoRow);

    // Keep a friendly empty state visible until the first task is queued.
    m_emptyLabel = new QLabel(
        QStringLiteral("Cuando prepares Minecraft o descargues contenido, aquí verás el progreso y la velocidad."), this);
    m_emptyLabel->setObjectName(QStringLiteral("EmptyState"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAccessibleName(QStringLiteral("No hay descargas activas"));
    layout->addWidget(m_emptyLabel, 1);

    m_list = new QListWidget(this);
    m_list->setAccessibleName(QStringLiteral("Lista de progreso de descargas"));
    m_list->hide();
    layout->addWidget(m_list, 1);
}

int DownloadsPage::upsertDownload(const QString& id, const QString& name,
                                   int progress, qint64 speed, const QString& status) {
    // Update existing item in place — no full list rebuild
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        if (item->data(Qt::UserRole).toString() == id) {
            item->setText(QStringLiteral("%1 — %2% — %3 — %4")
                              .arg(name)
                              .arg(progress)
                              .arg(formatSpeed(speed))
                              .arg(status));
            item->setToolTip(QStringLiteral("%1 · %2 % · %3 · %4")
                                 .arg(name).arg(progress).arg(formatSpeed(speed), status));
            m_emptyLabel->hide();
            m_list->show();
            m_countLabel->setText(formatDownloadCount(m_list->count()));
            return i;
        }
    }

    // Create new item
    auto* item = new QListWidgetItem(m_list);
    item->setData(Qt::UserRole, id);
    item->setText(QStringLiteral("%1 — %2% — %3 — %4")
                      .arg(name)
                      .arg(progress)
                      .arg(formatSpeed(speed))
                      .arg(status));
    item->setToolTip(QStringLiteral("%1 · %2 % · %3 · %4")
                         .arg(name).arg(progress).arg(formatSpeed(speed), status));
    m_emptyLabel->hide();
    m_list->show();
    m_countLabel->setText(formatDownloadCount(m_list->count()));
    return m_list->count() - 1;
}

void DownloadsPage::removeDownload(const QString& id) {
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toString() == id) {
            delete m_list->takeItem(i);
            break;
        }
    }
    m_countLabel->setText(formatDownloadCount(m_list->count()));
    const bool empty = m_list->count() == 0;
    m_list->setVisible(!empty);
    m_emptyLabel->setVisible(empty);
}

void DownloadsPage::setGlobalSpeed(qint64 bytesPerSec) {
    m_speedLabel->setText(formatSpeed(bytesPerSec));
}

} // namespace cub3::ui::pages
