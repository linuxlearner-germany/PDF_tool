#include <QtTest>

#include "document/SearchService.h"
#include "rendering/PdfRenderEngine.h"

namespace
{
class SearchRenderEngine final : public PdfRenderEngine
{
public:
    bool loadDocument(const QString &, const QByteArray &, const QByteArray &) override { return true; }
    void closeDocument() override {}
    bool isLoaded() const override { return true; }
    bool requiresPassword() const override { return false; }
    int pageCount() const override { return 2; }
    QImage renderPage(int, double) override { return {}; }
    QSizeF pageSizePoints(int) override { return {}; }
    QString pageLabel(int) override { return {}; }
    QStringList pageLabels() override { return {}; }
    PdfTextSelection buildTextSelection(int, const QRectF &) override { return {}; }
    QVector<PdfSearchHit> search(const QString &query) override
    {
        if (query == QStringLiteral("needle")) {
            return {
                {0, QRectF(1.0, 2.0, 3.0, 4.0), QStringLiteral("a")},
                {1, QRectF(5.0, 6.0, 7.0, 8.0), QStringLiteral("b")}
            };
        }
        return {};
    }
    QVector<PdfAnnotation> annotations(int) override { return {}; }
    QVector<PdfFormField> formFields(int) override { return {}; }
    PdfDocumentMetadata documentMetadata() const override { return {}; }
    QVector<PdfOutlineEntry> outlineEntries() const override { return {}; }
    QString lastError() const override { return {}; }
};
}

class SearchServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void queryAndNavigateAcrossPages();
};

void SearchServiceTests::queryAndNavigateAcrossPages()
{
    SearchRenderEngine engine;
    SearchService service;

    const SearchNavigationResult first = service.setQuery(engine, QStringLiteral("needle"));
    QVERIFY(first.hasResults);
    QCOMPARE(first.targetPageIndex, 0);
    QCOMPARE(service.hitCount(), 2);
    QCOMPARE(service.query(), QStringLiteral("needle"));

    const SearchNavigationResult next = service.findNext();
    QVERIFY(next.hasResults);
    QCOMPARE(next.targetPageIndex, 1);
    QCOMPARE(service.currentIndex(), 1);

    const QString cleared = service.clear();
    QCOMPARE(cleared, QStringLiteral("Suche zurückgesetzt."));
    QVERIFY(!service.hasResults());
}

QTEST_MAIN(SearchServiceTests)

#include "SearchServiceTests.moc"
