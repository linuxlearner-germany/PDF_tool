#include <QtTest>

#include "document/FormFieldModel.h"

class FormFieldModelTests : public QObject
{
    Q_OBJECT

private slots:
    void textStyleIsNormalizedAndSerialized();
    void readonlyTextFieldRejectsStyleChanges();
};

void FormFieldModelTests::textStyleIsNormalizedAndSerialized()
{
    FormFieldModel model;

    PdfFormField field;
    field.id = QStringLiteral("field-1");
    field.pageIndex = 0;
    field.kind = PdfFormFieldKind::Text;
    field.textValue = QStringLiteral("abc");
    model.setFields({ field });

    PdfTextStyle style;
    style.fontFamily = QStringLiteral("Fancy Serif");
    style.fontSize = 15.0;
    style.textColor = QColor(QStringLiteral("#336699"));
    QVERIFY(model.setTextStyle(field.id, style));

    const QJsonArray json = model.toJson();
    FormFieldModel restored;
    restored.setFields({ field });
    QVERIFY(restored.fromJson(json));

    PdfTextStyle restoredStyle;
    QVERIFY(restored.textFieldStyle(field.id, restoredStyle));
    QCOMPARE(restoredStyle.fontFamily, QStringLiteral("Helvetica"));
    QCOMPARE(restoredStyle.fontSize, 15.0);
    QCOMPARE(restoredStyle.textColor, QColor(QStringLiteral("#336699")));
}

void FormFieldModelTests::readonlyTextFieldRejectsStyleChanges()
{
    FormFieldModel model;

    PdfFormField field;
    field.id = QStringLiteral("field-2");
    field.pageIndex = 0;
    field.kind = PdfFormFieldKind::Text;
    field.readOnly = true;
    model.setFields({ field });

    PdfTextStyle style;
    style.fontFamily = QStringLiteral("Courier");
    style.fontSize = 10.0;
    style.textColor = QColor(Qt::red);

    QVERIFY(!model.setTextStyle(field.id, style));
}

QTEST_MAIN(FormFieldModelTests)

#include "FormFieldModelTests.moc"
