#pragma once

#include <functional>

#include <QByteArray>
#include <QSizeF>
#include <QString>
#include <QVector>

#include "document/PdfDocumentTypes.h"

class PdfRenderEngine;
class QPrinter;

enum class EditedPdfExportMode
{
    NativePdf,
    RasterizedPdf
};

struct EditedPdfExportDecision
{
    EditedPdfExportMode mode {EditedPdfExportMode::RasterizedPdf};
    QString reason;
};

class ExportService
{
public:
    CapabilityState capabilityState() const;
    EditedPdfExportDecision decideEditedPdfExport(
        const QVector<PdfAnnotation> &annotations,
        const QVector<PdfRedaction> &redactions,
        bool formStateChanged) const;

    bool printDocument(PdfRenderEngine &renderEngine, int pageCount, QPrinter *printer, QString &errorMessage) const;
    bool exportEditedPdf(
        PdfRenderEngine &renderEngine,
        const QString &documentPath,
        const QString &outputFile,
        const QVector<PdfAnnotation> &annotations,
        const QVector<PdfFormField> &formFields,
        const QVector<PdfFormField> &baseFormFields,
        const QVector<PdfRedaction> &redactions,
        const QByteArray &ownerPassword,
        const QByteArray &userPassword,
        const QString &excludedAnnotationId,
        QString &errorMessage,
        QString &statusMessage) const;
    bool exportRedactedPdf(
        PdfRenderEngine &renderEngine,
        const QString &documentPath,
        const QString &outputFile,
        const QVector<PdfRedaction> &redactions,
        int pageCount,
        const QByteArray &ownerPassword,
        const QByteArray &userPassword,
        QString &errorMessage,
        QString &statusMessage) const;
    bool saveDocumentState(
        const QString &documentPath,
        const QString &sidecarPath,
        const std::function<bool(const QString &, QString &, QString &)> &exportToPath,
        QString &errorMessage,
        QString &statusMessage) const;
};
