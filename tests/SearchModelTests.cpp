#include <QtTest>

#include "document/SearchModel.h"

class SearchModelTests : public QObject
{
    Q_OBJECT

private slots:
    void nextAndPreviousWrapAcrossHits();
    void pageRectQueriesRespectCurrentPage();
};

void SearchModelTests::nextAndPreviousWrapAcrossHits()
{
    SearchModel model;
    model.setResults(QStringLiteral("term"), {
        {0, QRectF(0.0, 0.0, 10.0, 10.0), QStringLiteral("a")},
        {1, QRectF(10.0, 0.0, 10.0, 10.0), QStringLiteral("b")},
    });

    QCOMPARE(model.currentIndex(), 0);
    QCOMPARE(model.next()->pageIndex, 1);
    QCOMPARE(model.next()->pageIndex, 0);
    QCOMPARE(model.previous()->pageIndex, 1);
}

void SearchModelTests::pageRectQueriesRespectCurrentPage()
{
    SearchModel model;
    model.setResults(QStringLiteral("term"), {
        {0, QRectF(0.0, 0.0, 10.0, 10.0), QStringLiteral("a")},
        {0, QRectF(20.0, 0.0, 10.0, 10.0), QStringLiteral("b")},
        {1, QRectF(5.0, 5.0, 5.0, 5.0), QStringLiteral("c")},
    });

    QCOMPARE(model.hitRectsForPage(0).size(), 2);
    QCOMPARE(model.currentHitRectsForPage(0).size(), 1);
    QCOMPARE(model.currentHitRectsForPage(1).size(), 0);
}

QTEST_MAIN(SearchModelTests)

#include "SearchModelTests.moc"
