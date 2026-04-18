#include <QtTest>

#include "document/RedactionModel.h"
#include "document/SelectionService.h"
#include "rendering/PdfRenderEngine.h"

namespace
{
class SelectionRenderEngine final : public PdfRenderEngine
{
public:
    bool loadDocument(const QString &, const QByteArray &, const QByteArray &) override { return true; }
    void closeDocument() override {}
    bool isLoaded() const override { return true; }
    bool requiresPassword() const override { return false; }
    int pageCount() const override { return 1; }
    QImage renderPage(int, double) override { return {}; }
    QSizeF pageSizePoints(int) override { return QSizeF(100.0, 100.0); }
    QString pageLabel(int) override { return {}; }
    QStringList pageLabels() override { return {}; }
    PdfTextSelection buildTextSelection(int, const QRectF &pageRect) override
    {
        PdfTextSelection selection;
        PdfTextSelectionFragment fragment;
        fragment.pageRect = pageRect;
        fragment.text = QStringLiteral("selected");
        selection.fragments.append(fragment);
        return selection;
    }
    QVector<PdfSearchHit> search(const QString &) override { return {}; }
    QVector<PdfAnnotation> annotations(int) override { return {}; }
    QVector<PdfFormField> formFields(int) override { return {}; }
    PdfDocumentMetadata documentMetadata() const override { return {}; }
    QVector<PdfOutlineEntry> outlineEntries() const override { return {}; }
    QString lastError() const override { return {}; }
};
}

class SelectionServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void selectionStateAndRedactionLifecycle();
};

void SelectionServiceTests::selectionStateAndRedactionLifecycle()
{
    SelectionRenderEngine engine;
    SelectionService service;
    RedactionModel redactionModel;

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QVector<QRectF> highlightRects;
    QVERIFY(service.updateTextSelection(
        engine,
        image,
        0,
        QRectF(10.0, 10.0, 20.0, 20.0),
        [](const QRectF &rect) { return rect; },
        &highlightRects));
    QVERIFY(service.hasTextSelection());
    QVERIFY(service.hasAreaSelection());
    QCOMPARE(service.selectedText(), QStringLiteral("selected"));
    QCOMPARE(highlightRects.size(), 1);

    QVERIFY(service.addRedactionFromSelection(0, redactionModel));
    QVERIFY(redactionModel.hasRedactions());

    service.clearTextSelection();
    QVERIFY(!service.hasTextSelection());
    QVERIFY(!service.hasAreaSelection());
}

QTEST_MAIN(SelectionServiceTests)

#include "SelectionServiceTests.moc"
