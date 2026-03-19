#include "ui/PageThumbnailListModel.h"

#include <QIcon>

#include "document/PdfDocumentController.h"

PageThumbnailListModel::PageThumbnailListModel(PdfDocumentController *controller, QObject *parent)
    : QAbstractListModel(parent)
    , m_controller(controller)
{
    Q_ASSERT(m_controller);

    connect(m_controller, &PdfDocumentController::documentStateChanged, this, [this](bool) {
        beginResetModel();
        endResetModel();
    });
}

int PageThumbnailListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_controller || !m_controller->hasDocument()) {
        return 0;
    }

    return m_controller->pageCount();
}

QVariant PageThumbnailListModel::data(const QModelIndex &index, int role) const
{
    if (!m_controller || !index.isValid() || index.row() < 0 || index.row() >= m_controller->pageCount()) {
        return {};
    }

    const int pageIndex = index.row();
    switch (role) {
    case Qt::DisplayRole:
        return QStringLiteral("%1").arg(pageIndex + 1);
    case Qt::DecorationRole:
        return QIcon(m_controller->thumbnailForPage(pageIndex, m_thumbnailSize));
    case Qt::ToolTipRole:
        return QStringLiteral("Seite %1 (%2)")
            .arg(pageIndex + 1)
            .arg(m_controller->pageLabels().value(pageIndex));
    case Qt::SizeHintRole:
        return QSize(m_thumbnailSize.width() + 28, m_thumbnailSize.height() + 36);
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
    default:
        return {};
    }
}

void PageThumbnailListModel::setThumbnailSize(const QSize &size)
{
    if (size.isValid() && size != m_thumbnailSize) {
        beginResetModel();
        m_thumbnailSize = size;
        if (m_controller) {
            m_controller->setThumbnailSize(size);
        }
        endResetModel();
    }
}

QSize PageThumbnailListModel::thumbnailSize() const
{
    return m_thumbnailSize;
}

void PageThumbnailListModel::refresh()
{
    beginResetModel();
    endResetModel();
}
