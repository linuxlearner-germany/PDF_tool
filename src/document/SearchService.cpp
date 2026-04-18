#include "document/SearchService.h"

#include "rendering/PdfRenderEngine.h"

void SearchService::reset()
{
    m_searchModel.clear();
}

bool SearchService::hasResults() const
{
    return m_searchModel.hasResults();
}

QString SearchService::query() const
{
    return m_searchModel.query();
}

int SearchService::currentIndex() const
{
    return m_searchModel.currentIndex();
}

int SearchService::hitCount() const
{
    return m_searchModel.hitCount();
}

const PdfSearchHit *SearchService::currentHit() const
{
    return m_searchModel.currentHit();
}

QVector<QRectF> SearchService::hitRectsForPage(int pageIndex) const
{
    return m_searchModel.hitRectsForPage(pageIndex);
}

QVector<QRectF> SearchService::currentHitRectsForPage(int pageIndex) const
{
    return m_searchModel.currentHitRectsForPage(pageIndex);
}

SearchNavigationResult SearchService::setQuery(PdfRenderEngine &renderEngine, const QString &query)
{
    const QString trimmedQuery = query.trimmed();
    if (trimmedQuery.isEmpty()) {
        return {false, false, -1, clear()};
    }

    m_searchModel.setResults(trimmedQuery, renderEngine.search(trimmedQuery));
    if (!m_searchModel.hasResults()) {
        SearchNavigationResult result;
        result.fallbackMessage = QStringLiteral("Keine Treffer für \"%1\".").arg(trimmedQuery);
        return result;
    }

    return navigationResultForCurrentHit();
}

SearchNavigationResult SearchService::findNext()
{
    if (!m_searchModel.next()) {
        return {};
    }
    return navigationResultForCurrentHit();
}

SearchNavigationResult SearchService::findPrevious()
{
    if (!m_searchModel.previous()) {
        return {};
    }
    return navigationResultForCurrentHit();
}

QString SearchService::clear()
{
    m_searchModel.clear();
    return QStringLiteral("Suche zurückgesetzt.");
}

SearchNavigationResult SearchService::navigationResultForCurrentHit() const
{
    SearchNavigationResult result;
    result.hasResults = m_searchModel.hasResults();
    if (const PdfSearchHit *hit = m_searchModel.currentHit()) {
        result.targetPageIndex = hit->pageIndex;
    }
    return result;
}
