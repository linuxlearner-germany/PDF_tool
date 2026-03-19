#pragma once

#include <utility>

#include <QVector>

#include "document/PdfDocumentTypes.h"

class SearchModel
{
public:
    void setResults(const QString &query, QVector<PdfSearchHit> hits)
    {
        m_query = query;
        m_hits = std::move(hits);
        m_currentIndex = m_hits.isEmpty() ? -1 : 0;
    }

    void clear()
    {
        m_query.clear();
        m_hits.clear();
        m_currentIndex = -1;
    }

    bool hasResults() const
    {
        return !m_hits.isEmpty();
    }

    QString query() const
    {
        return m_query;
    }

    int currentIndex() const
    {
        return m_currentIndex;
    }

    int hitCount() const
    {
        return m_hits.size();
    }

    const PdfSearchHit *currentHit() const
    {
        if (m_currentIndex < 0 || m_currentIndex >= m_hits.size()) {
            return nullptr;
        }

        return &m_hits.at(m_currentIndex);
    }

    const PdfSearchHit *next()
    {
        if (m_hits.isEmpty()) {
            return nullptr;
        }

        m_currentIndex = (m_currentIndex + 1) % m_hits.size();
        return &m_hits.at(m_currentIndex);
    }

    const PdfSearchHit *previous()
    {
        if (m_hits.isEmpty()) {
            return nullptr;
        }

        m_currentIndex = (m_currentIndex - 1 + m_hits.size()) % m_hits.size();
        return &m_hits.at(m_currentIndex);
    }

    QVector<QRectF> hitRectsForPage(int pageIndex) const
    {
        QVector<QRectF> rects;
        for (const PdfSearchHit &hit : m_hits) {
            if (hit.pageIndex == pageIndex) {
                rects.append(hit.pageRect);
            }
        }
        return rects;
    }

    QVector<QRectF> currentHitRectsForPage(int pageIndex) const
    {
        QVector<QRectF> rects;
        if (const PdfSearchHit *hit = currentHit(); hit && hit->pageIndex == pageIndex) {
            rects.append(hit->pageRect);
        }
        return rects;
    }

private:
    QString m_query;
    QVector<PdfSearchHit> m_hits;
    int m_currentIndex {-1};
};
