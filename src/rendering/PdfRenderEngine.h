#pragma once

#include <QImage>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVector>

#include "document/PdfDocumentTypes.h"
#include "document/PdfTextSelection.h"

class PdfRenderEngine
{
public:
    virtual ~PdfRenderEngine() = default;

    virtual bool loadDocument(
        const QString &filePath,
        const QByteArray &ownerPassword = QByteArray(),
        const QByteArray &userPassword = QByteArray()) = 0;
    virtual void closeDocument() = 0;
    virtual bool isLoaded() const = 0;
    virtual bool requiresPassword() const = 0;
    virtual int pageCount() const = 0;
    virtual QImage renderPage(int pageIndex, double scale) = 0;
    virtual QSizeF pageSizePoints(int pageIndex) = 0;
    virtual QString pageLabel(int pageIndex) = 0;
    virtual QStringList pageLabels() = 0;
    virtual PdfTextSelection buildTextSelection(int pageIndex, const QRectF &pageRect) = 0;
    virtual QVector<PdfSearchHit> search(const QString &query) = 0;
    virtual QVector<PdfAnnotation> annotations(int pageIndex) = 0;
    virtual QVector<PdfFormField> formFields(int pageIndex) = 0;
    virtual PdfDocumentMetadata documentMetadata() const = 0;
    virtual QVector<PdfOutlineEntry> outlineEntries() const = 0;
    virtual QString lastError() const = 0;
};
