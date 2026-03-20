#pragma once

#include <memory>

#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>

#include "rendering/PdfRenderEngine.h"

namespace Poppler
{
class Document;
class Page;
class OutlineItem;
}

class PopplerAdapter final : public PdfRenderEngine
{
public:
    PopplerAdapter();
    ~PopplerAdapter() override;

    bool loadDocument(
        const QString &filePath,
        const QByteArray &ownerPassword = QByteArray(),
        const QByteArray &userPassword = QByteArray()) override;
    void closeDocument() override;
    bool isLoaded() const override;
    bool requiresPassword() const override;
    int pageCount() const override;
    QImage renderPage(int pageIndex, double scale) override;
    QSizeF pageSizePoints(int pageIndex) override;
    QString pageLabel(int pageIndex) override;
    QStringList pageLabels() override;
    PdfTextSelection buildTextSelection(int pageIndex, const QRectF &pageRect) override;
    QVector<PdfSearchHit> search(const QString &query) override;
    QVector<PdfAnnotation> annotations(int pageIndex) override;
    QVector<PdfFormField> formFields(int pageIndex) override;
    PdfDocumentMetadata documentMetadata() const override;
    QVector<PdfOutlineEntry> outlineEntries() const override;
    QString lastError() const override;

private:
    std::unique_ptr<Poppler::Page> loadPage(int pageIndex);
    PdfOutlineEntry convertOutlineItem(const Poppler::OutlineItem &item) const;

    std::unique_ptr<Poppler::Document> m_document;
    QString m_lastError;
    bool m_requiresPassword {false};
};
