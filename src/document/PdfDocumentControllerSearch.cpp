#include "document/PdfDocumentController.h"

#include "document/SearchService.h"
#include "rendering/PdfRenderEngine.h"

void PdfDocumentController::setSearchQuery(const QString &query)
{
    if (!hasDocument()) {
        return;
    }

    emit busyStateChanged(true, QStringLiteral("Suche läuft..."));
    const SearchNavigationResult result = m_searchService->setQuery(*m_renderEngine, query);
    emit busyStateChanged(false, QString());
    if (!result.hasResults) {
        emitOverlayState();
        emitSearchState(result.fallbackMessage.isEmpty() ? QStringLiteral("Keine aktive Suche.") : result.fallbackMessage);
        return;
    }

    if (result.targetPageIndex != m_currentPageIndex) {
        m_currentPageIndex = result.targetPageIndex;
        renderCurrentPage();
    } else {
        emitOverlayState();
    }
    emitSearchState();
}

void PdfDocumentController::findNext()
{
    const SearchNavigationResult result = m_searchService->findNext();
    if (!result.hasResults) {
        return;
    }

    if (result.targetPageIndex != m_currentPageIndex) {
        m_currentPageIndex = result.targetPageIndex;
        renderCurrentPage();
    } else {
        emitOverlayState();
    }
    emitSearchState();
}

void PdfDocumentController::findPrevious()
{
    const SearchNavigationResult result = m_searchService->findPrevious();
    if (!result.hasResults) {
        return;
    }

    if (result.targetPageIndex != m_currentPageIndex) {
        m_currentPageIndex = result.targetPageIndex;
        renderCurrentPage();
    } else {
        emitOverlayState();
    }
    emitSearchState();
}

void PdfDocumentController::clearSearch()
{
    const QString statusMessage = m_searchService->clear();
    emitOverlayState();
    emitSearchState(statusMessage);
}
