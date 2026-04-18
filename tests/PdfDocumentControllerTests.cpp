#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "document/PdfDocumentController.h"
#include "rendering/PdfRenderEngine.h"

namespace
{
class FakeRenderEngine final : public PdfRenderEngine
{
public:
    bool loadDocument(const QString &filePath, const QByteArray &, const QByteArray &) override
    {
        m_loaded = !filePath.isEmpty();
        m_filePath = filePath;
        return m_loaded;
    }

    void closeDocument() override
    {
        m_loaded = false;
    }

    bool isLoaded() const override
    {
        return m_loaded;
    }

    bool requiresPassword() const override
    {
        return false;
    }

    int pageCount() const override
    {
        return 1;
    }

    QImage renderPage(int, double) override
    {
        QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        return image;
    }

    QSizeF pageSizePoints(int) override
    {
        return QSizeF(100.0, 100.0);
    }

    QString pageLabel(int) override
    {
        return QStringLiteral("1");
    }

    QStringList pageLabels() override
    {
        return { QStringLiteral("1") };
    }

    PdfTextSelection buildTextSelection(int, const QRectF &) override
    {
        return {};
    }

    QVector<PdfSearchHit> search(const QString &) override
    {
        return {};
    }

    QVector<PdfAnnotation> annotations(int) override
    {
        return {};
    }

    QVector<PdfFormField> formFields(int) override
    {
        return {};
    }

    PdfDocumentMetadata documentMetadata() const override
    {
        PdfDocumentMetadata metadata;
        metadata.filePath = m_filePath;
        metadata.pageCount = 1;
        return metadata;
    }

    QVector<PdfOutlineEntry> outlineEntries() const override
    {
        return {};
    }

    QString lastError() const override
    {
        return {};
    }

private:
    bool m_loaded {false};
    QString m_filePath;
};
}

class PdfDocumentControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void undoRedoRestoresRedactions();
};

void PdfDocumentControllerTests::undoRedoRestoresRedactions()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString documentPath = tempDir.filePath(QStringLiteral("sample.pdf"));
    QFile file(documentPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("%PDF-1.7");
    file.close();

    PdfDocumentController controller(std::make_unique<FakeRenderEngine>());
    QVERIFY(controller.openDocument(documentPath));
    QVERIFY(!controller.hasRedactions());

    controller.updateTextSelection(QRectF(10.0, 10.0, 20.0, 20.0));
    QVERIFY(controller.hasAreaSelection());

    controller.addRedactionFromSelection();
    QVERIFY(controller.hasRedactions());
    QVERIFY(controller.canUndo());
    QVERIFY(!controller.canRedo());

    controller.undo();
    QVERIFY(!controller.hasRedactions());
    QVERIFY(controller.canRedo());

    controller.redo();
    QVERIFY(controller.hasRedactions());
}

QTEST_MAIN(PdfDocumentControllerTests)

#include "PdfDocumentControllerTests.moc"
