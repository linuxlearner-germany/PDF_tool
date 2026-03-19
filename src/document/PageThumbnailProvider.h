#pragma once

#include <QHash>
#include <QPixmap>
#include <QSize>

class PdfRenderEngine;

class PageThumbnailProvider
{
public:
    explicit PageThumbnailProvider(PdfRenderEngine *renderEngine);

    void clear();
    void setThumbnailSize(const QSize &size);
    QPixmap thumbnailForPage(int pageIndex);

private:
    PdfRenderEngine *m_renderEngine {nullptr};
    QSize m_thumbnailSize {140, 180};
    QHash<int, QPixmap> m_cache;
};
