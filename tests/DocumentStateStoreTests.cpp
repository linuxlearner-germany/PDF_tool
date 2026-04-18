#include <QtTest>

#include <QTemporaryDir>

#include "document/AnnotationModel.h"
#include "document/DocumentStateStore.h"
#include "document/FormFieldModel.h"
#include "document/RedactionModel.h"

class DocumentStateStoreTests : public QObject
{
    Q_OBJECT

private slots:
    void versionedStateRoundTripsThroughDisk();
    void legacyAnnotationSchemaStillLoads();
};

void DocumentStateStoreTests::versionedStateRoundTripsThroughDisk()
{
    AnnotationModel annotations;
    QVERIFY(annotations.addNote(0, QRectF(5.0, 5.0, 20.0, 20.0), QStringLiteral("note")));

    FormFieldModel formFields;
    PdfFormField field;
    field.id = QStringLiteral("field-1");
    field.pageIndex = 0;
    field.kind = PdfFormFieldKind::Text;
    formFields.setFields({field});
    QVERIFY(formFields.setTextValue(field.id, QStringLiteral("value")));

    RedactionModel redactions;
    QVERIFY(redactions.add(1, QRectF(1.0, 2.0, 3.0, 4.0)));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sidecarPath = tempDir.filePath(QStringLiteral("sample.pdf.annotations.json"));

    const QJsonObject writtenState = DocumentStateStore::buildState(annotations, formFields, redactions, 3);
    QString errorMessage;
    QVERIFY(DocumentStateStore::saveToFile(sidecarPath, writtenState, &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    QJsonObject loadedState;
    QVERIFY(DocumentStateStore::loadFromFile(sidecarPath, loadedState, &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    AnnotationModel restoredAnnotations;
    FormFieldModel restoredFormFields;
    restoredFormFields.setFields({field});
    RedactionModel restoredRedactions;
    QVERIFY(DocumentStateStore::applyState(
        loadedState,
        restoredAnnotations,
        restoredFormFields,
        restoredRedactions,
        &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));

    QCOMPARE(restoredAnnotations.annotations().size(), 1);
    QCOMPARE(restoredFormFields.fields().first().textValue, QStringLiteral("value"));
    QCOMPARE(restoredRedactions.redactions().size(), 1);
}

void DocumentStateStoreTests::legacyAnnotationSchemaStillLoads()
{
    AnnotationModel annotations;
    QVERIFY(annotations.addRectangle(0, QRectF(10.0, 10.0, 15.0, 15.0)));

    QJsonObject legacyRoot;
    legacyRoot.insert(QStringLiteral("annotations"), annotations.toJson());

    AnnotationModel restoredAnnotations;
    FormFieldModel restoredFormFields;
    restoredFormFields.setFields({});
    RedactionModel restoredRedactions;
    QString errorMessage;
    QVERIFY(DocumentStateStore::applyState(
        legacyRoot,
        restoredAnnotations,
        restoredFormFields,
        restoredRedactions,
        &errorMessage));
    QVERIFY2(errorMessage.isEmpty(), qPrintable(errorMessage));
    QCOMPARE(restoredAnnotations.annotations().size(), 1);
    QVERIFY(restoredRedactions.redactions().isEmpty());
}

QTEST_MAIN(DocumentStateStoreTests)

#include "DocumentStateStoreTests.moc"
