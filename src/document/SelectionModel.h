#pragma once

#include <QVector>

#include "document/PdfTextSelection.h"

class SelectionModel
{
public:
    void setSelection(int pageIndex, const PdfTextSelection &selection)
    {
        m_pageIndex = pageIndex;
        m_selection = selection;
    }

    void clear()
    {
        m_pageIndex = -1;
        m_selection = {};
    }

    bool isEmpty() const
    {
        return m_selection.isEmpty();
    }

    int pageIndex() const
    {
        return m_pageIndex;
    }

    QString plainText() const
    {
        return m_selection.toPlainText();
    }

    const PdfTextSelection &selection() const
    {
        return m_selection;
    }

    QVector<QRectF> pageRects() const
    {
        QVector<QRectF> rects;
        rects.reserve(m_selection.fragments.size());
        for (const PdfTextSelectionFragment &fragment : m_selection.fragments) {
            rects.append(fragment.pageRect);
        }
        return rects;
    }

private:
    int m_pageIndex {-1};
    PdfTextSelection m_selection;
};
