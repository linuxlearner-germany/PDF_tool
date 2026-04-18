#include <QtTest>

#include "document/ExportService.h"

class ExportServiceTests : public QObject
{
    Q_OBJECT

private slots:
    void nativeCapabilityDrivesEditedExportDecision();
    void redactionsForceRasterizedFallback();
};

void ExportServiceTests::nativeCapabilityDrivesEditedExportDecision()
{
    ExportService service;

    PdfAnnotation annotation;
    annotation.kind = PdfAnnotationKind::Highlight;

    const EditedPdfExportDecision decision = service.decideEditedPdfExport({annotation}, {}, false);
    if (service.capabilityState().available) {
        QCOMPARE(decision.mode, EditedPdfExportMode::NativePdf);
    } else {
        QCOMPARE(decision.mode, EditedPdfExportMode::RasterizedPdf);
        QVERIFY(!decision.reason.isEmpty());
    }
}

void ExportServiceTests::redactionsForceRasterizedFallback()
{
    ExportService service;

    PdfAnnotation annotation;
    annotation.kind = PdfAnnotationKind::Highlight;

    PdfRedaction redaction;
    redaction.pageIndex = 0;
    redaction.pageRect = QRectF(0.0, 0.0, 10.0, 10.0);

    const EditedPdfExportDecision decision = service.decideEditedPdfExport({annotation}, {redaction}, true);
    QCOMPARE(decision.mode, EditedPdfExportMode::RasterizedPdf);
}

QTEST_MAIN(ExportServiceTests)

#include "ExportServiceTests.moc"
