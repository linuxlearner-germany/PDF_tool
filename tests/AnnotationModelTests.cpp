#include <QtTest>

#include "document/AnnotationModel.h"

class AnnotationModelTests : public QObject
{
    Q_OBJECT

private slots:
    void freeTextStyleIsNormalizedOnInsert();
    void signatureResizePreservesAspectRatio();
};

void AnnotationModelTests::freeTextStyleIsNormalizedOnInsert()
{
    AnnotationModel model;

    PdfTextStyle style;
    style.fontFamily = QStringLiteral("Some Random Sans");
    style.fontSize = -5.0;

    QVERIFY(model.addFreeText(0, QRectF(10.0, 10.0, 120.0, 40.0), QStringLiteral("Hello"), style));

    const QVector<PdfAnnotation> annotations = model.annotations();
    QCOMPARE(annotations.size(), 1);
    QCOMPARE(annotations.first().textStyle.fontFamily, QStringLiteral("Helvetica"));
    QCOMPARE(annotations.first().textStyle.fontSize, 12.0);
    QCOMPARE(annotations.first().textStyle.textColor, QColor(Qt::black));
}

void AnnotationModelTests::signatureResizePreservesAspectRatio()
{
    AnnotationModel model;
    QVERIFY(model.addSignature(0, QRectF(5.0, 5.0, 200.0, 100.0), QByteArray("png")));

    QVERIFY(model.resizeSelectedSignature(QSizeF(100.0, 10.0)));

    const QVector<PdfAnnotation> annotations = model.annotations();
    QCOMPARE(annotations.size(), 1);
    const QRectF rect = annotations.first().pageRects.first();
    QCOMPARE(rect.width(), 300.0);
    QCOMPARE(rect.height(), 150.0);
}

QTEST_MAIN(AnnotationModelTests)

#include "AnnotationModelTests.moc"
