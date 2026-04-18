#pragma once

#include <QRectF>
#include <QString>
#include <QVector>

#include "document/SearchModel.h"

class PdfRenderEngine;

struct SearchNavigationResult
{
    bool hasResults {false};
    bool changedPage {false};
    int targetPageIndex {-1};
    QString fallbackMessage;
};

class SearchService
{
public:
    void reset();

    bool hasResults() const;
    QString query() const;
    int currentIndex() const;
    int hitCount() const;
    const PdfSearchHit *currentHit() const;
    QVector<QRectF> hitRectsForPage(int pageIndex) const;
    QVector<QRectF> currentHitRectsForPage(int pageIndex) const;

    SearchNavigationResult setQuery(PdfRenderEngine &renderEngine, const QString &query);
    SearchNavigationResult findNext();
    SearchNavigationResult findPrevious();
    QString clear();

private:
    SearchNavigationResult navigationResultForCurrentHit() const;

    SearchModel m_searchModel;
};
