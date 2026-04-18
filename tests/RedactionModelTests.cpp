#include <QtTest>

#include "document/RedactionModel.h"

class RedactionModelTests : public QObject
{
    Q_OBJECT

private slots:
    void redactionsRoundTripThroughJson();
    void remapPagesDropsDeletedPages();
};

void RedactionModelTests::redactionsRoundTripThroughJson()
{
    RedactionModel model;
    QVERIFY(model.add(2, QRectF(10.0, 12.0, 40.0, 18.0), QStringLiteral("secret")));

    const QJsonArray json = model.toJson();
    RedactionModel restored;
    QVERIFY(restored.fromJson(json));

    const QVector<PdfRedaction> redactions = restored.redactions();
    QCOMPARE(redactions.size(), 1);
    QCOMPARE(redactions.first().pageIndex, 2);
    QCOMPARE(redactions.first().reason, QStringLiteral("secret"));
    QCOMPARE(redactions.first().pageRect, QRectF(10.0, 12.0, 40.0, 18.0));
}

void RedactionModelTests::remapPagesDropsDeletedPages()
{
    RedactionModel model;
    QVERIFY(model.add(0, QRectF(0.0, 0.0, 10.0, 10.0)));
    QVERIFY(model.add(2, QRectF(5.0, 5.0, 10.0, 10.0)));

    QVERIFY(model.remapPages({2, 0}));

    const QVector<PdfRedaction> redactions = model.redactions();
    QCOMPARE(redactions.size(), 2);
    QCOMPARE(redactions.at(0).pageIndex, 1);
    QCOMPARE(redactions.at(1).pageIndex, 0);
}

QTEST_MAIN(RedactionModelTests)

#include "RedactionModelTests.moc"
