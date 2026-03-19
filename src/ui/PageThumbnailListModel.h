#pragma once

#include <QAbstractListModel>
#include <QModelIndex>
#include <QSize>

class PdfDocumentController;

class PageThumbnailListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PageThumbnailListModel(PdfDocumentController *controller, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setThumbnailSize(const QSize &size);
    QSize thumbnailSize() const;
    void refresh();

private:
    PdfDocumentController *m_controller {nullptr};
    QSize m_thumbnailSize {140, 180};
};
