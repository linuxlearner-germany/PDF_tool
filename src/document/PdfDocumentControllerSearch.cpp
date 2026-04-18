#include "document/PdfDocumentController.h"

#include "rendering/PdfRenderEngine.h"

void PdfDocumentController::setSearchQuery(const QString &query)
{
    if (!hasDocument()) {
        return;
    }

    const QString trimmedQuery = query.trimmed();
    if (trimmedQuery.isEmpty()) {
        clearSearch();
        return;
    }

    emit busyStateChanged(true, QStringLiteral("Suche läuft..."));
    m_searchModel.setResults(trimmedQuery, m_renderEngine->search(trimmedQuery));
    emit busyStateChanged(false, QString());
    if (!m_searchModel.hasResults()) {
        emitOverlayState();
        emitSearchState(QStringLiteral("Keine Treffer für \"%1\".").arg(trimmedQuery));
        return;
    }

    if (const PdfSearchHit *hit = m_searchModel.currentHit()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
    }

    emitSearchState();
}

void PdfDocumentController::findNext()
{
    if (const PdfSearchHit *hit = m_searchModel.next()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
        emitSearchState();
    }
}

void PdfDocumentController::findPrevious()
{
    if (const PdfSearchHit *hit = m_searchModel.previous()) {
        if (hit->pageIndex != m_currentPageIndex) {
            m_currentPageIndex = hit->pageIndex;
            renderCurrentPage();
        } else {
            emitOverlayState();
        }
        emitSearchState();
    }
}

void PdfDocumentController::clearSearch()
{
    m_searchModel.clear();
    emitOverlayState();
    emitSearchState(QStringLiteral("Suche zurückgesetzt."));
}
