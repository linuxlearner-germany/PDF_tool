#include "document/PageThumbnailProvider.h"

#include <algorithm>

#include <QImage>

#include "rendering/PdfRenderEngine.h"

PageThumbnailProvider::PageThumbnailProvider(PdfRenderEngine *renderEngine)
    : m_renderEngine(renderEngine)
{
}

void PageThumbnailProvider::clear()
{
    m_cache.clear();
}

void PageThumbnailProvider::setThumbnailSize(const QSize &size)
{
    if (size.isValid() && size != m_thumbnailSize) {
        m_thumbnailSize = size;
        clear();
    }
}

QPixmap PageThumbnailProvider::thumbnailForPage(int pageIndex)
{
    if (!m_renderEngine || pageIndex < 0) {
        return {};
    }

    if (const auto it = m_cache.constFind(pageIndex); it != m_cache.cend()) {
        return it.value();
    }

    const QSizeF pageSize = m_renderEngine->pageSizePoints(pageIndex);
    if (!pageSize.isValid() || pageSize.isEmpty()) {
        return {};
    }

    const double scaleX = static_cast<double>(m_thumbnailSize.width()) / pageSize.width();
    const double scaleY = static_cast<double>(m_thumbnailSize.height()) / pageSize.height();
    const double renderScale = std::max(0.1, std::min(scaleX, scaleY));

    const QImage renderedPage = m_renderEngine->renderPage(pageIndex, renderScale);
    if (renderedPage.isNull()) {
        return {};
    }

    const QPixmap thumbnail = QPixmap::fromImage(
        renderedPage.scaled(m_thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_cache.insert(pageIndex, thumbnail);
    return thumbnail;
}
