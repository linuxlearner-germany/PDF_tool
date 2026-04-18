#pragma once

#include <memory>

#include <QByteArray>
#include <QFutureWatcher>
#include <QObject>
#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>

#include "document/AnnotationModel.h"
#include "document/FormFieldModel.h"
#include "document/PdfDocumentTypes.h"
#include "document/RedactionModel.h"
#include "document/SelectionModel.h"
#include "document/SearchModel.h"

class PdfRenderEngine;
class QPrinter;
class PageThumbnailProvider;

struct OcrJobResult
{
    QString text;
    QString errorMessage;
};

class PdfDocumentController : public QObject
{
    Q_OBJECT

public:
    explicit PdfDocumentController(std::unique_ptr<PdfRenderEngine> renderEngine, QObject *parent = nullptr);
    ~PdfDocumentController() override;

    bool openDocument(
        const QString &filePath,
        const QByteArray &ownerPassword = QByteArray(),
        const QByteArray &userPassword = QByteArray());
    void closeDocument();

    bool hasDocument() const;
    QString documentPath() const;
    int pageCount() const;
    int currentPageIndex() const;
    QString currentPageLabel() const;
    QStringList pageLabels() const;
    QVector<PdfOutlineEntry> outlineEntries() const;
    PdfDocumentMetadata metadata() const;
    double zoomFactor() const;
    QString lastError() const;
    QImage currentPageImage() const;
    bool hasTextSelection() const;
    bool hasAreaSelection() const;
    QString selectedText() const;
    bool isOcrAvailable() const;
    bool isOcrBusy() const;
    QString ocrAvailabilityError() const;
    bool hasSearchResults() const;
    QString searchQuery() const;
    bool requiresPassword() const;
    QByteArray currentUserPassword() const;
    QByteArray currentOwnerPassword() const;
    bool printDocument(QPrinter *printer);
    bool exportEditedPdf(const QString &outputFile, bool excludeSelectedSignatureAnnotation = false);
    bool exportRedactedPdf(const QString &outputFile);
    bool supportsNativePdfEditExport() const;
    QString nativePdfEditExportError() const;
    bool hasSelectedOverlay() const;
    PdfAnnotationKind selectedAnnotationKind() const;
    QColor selectedAnnotationColor() const;
    bool hasRedactions() const;
    bool hasTextEditAnnotations() const;
    bool hasSelectedNoteAnnotation() const;
    QString selectedNoteText() const;
    bool hasSelectedTextEditAnnotation() const;
    QString selectedTextEditText() const;
    PdfTextStyle selectedTextEditStyle() const;
    QColor selectedTextEditBackgroundColor() const;
    bool hasSelectedSignatureAnnotation() const;
    bool selectedSignatureAnnotationData(int &pageIndex, QRectF &pageRect, QByteArray &imageData) const;
    bool hasSelectedMovableAnnotation() const;
    bool textFormFieldStyleAt(const QPointF &imagePoint, QString &fieldId, QString &label, PdfTextStyle &style) const;
    bool canUndo() const;
    bool canRedo() const;
    QPixmap thumbnailForPage(int pageIndex, const QSize &targetSize);
    void setThumbnailSize(const QSize &size);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoomFactor(double factor);

    void goToPage(int pageIndex);
    void nextPage();
    void previousPage();

    void updateTextSelection(const QRectF &imageSelectionRect);
    void clearTextSelection();
    void copySelectedText();

    void addHighlightAnnotationFromSelection();
    void addRectangleAnnotationFromSelection();
    void addNoteAnnotationAt(const QPointF &imagePoint, const QString &noteText);
    void addFreeTextAt(
        const QPointF &imagePoint,
        const QString &text,
        const PdfTextStyle &style = PdfTextStyle(),
        const QColor &backgroundColor = QColor(255, 255, 255, 230));
    void replaceSelectedText(
        const QString &text,
        const PdfTextStyle &style = PdfTextStyle(),
        const QColor &backgroundColor = QColor(255, 255, 255, 230));
    void addSignatureFromImageAt(const QPointF &imagePoint, const QImage &signatureImage);
    void addSignatureFromImageToSelection(const QImage &signatureImage);
    void moveSelectedSignatureBy(const QPointF &imageDelta);
    void resizeSelectedSignatureBy(const QPointF &imageDelta);
    void resizeSelectedTextEditBy(const QPointF &imageDelta);
    bool remapPageOrder(const QVector<int> &newOrder);
    void selectOverlayAt(const QPointF &imagePoint);
    void deleteSelectedOverlay();
    void setSelectedAnnotationColor(const QColor &color);
    void updateSelectedNoteText(const QString &text);
    void updateSelectedTextEdit(
        const QString &text,
        const PdfTextStyle &style = PdfTextStyle(),
        const QColor &backgroundColor = QColor(255, 255, 255, 230));
    void saveDocumentState();
    void undo();
    void redo();

    void setSearchQuery(const QString &query);
    void findNext();
    void findPrevious();
    void clearSearch();

    void setFormFieldText(const QString &fieldId, const QString &text);
    void setFormFieldTextStyle(const QString &fieldId, const PdfTextStyle &style);
    void setFormFieldChecked(const QString &fieldId, bool checked);

    void addRedactionFromSelection();
    void runOcrOnCurrentPage();
    void runOcrOnSelection();

signals:
    void documentStateChanged(bool hasDocument);
    void pageImageChanged(const QImage &image);
    void zoomChanged(double factor);
    void currentPageChanged(int pageIndex, int pageCount, const QString &pageLabel);
    void selectionHighlightChanged(const QVector<QRectF> &imageRects);
    void searchHighlightChanged(const QVector<QRectF> &allImageRects, const QVector<QRectF> &currentImageRects);
    void annotationOverlaysChanged(const QVector<PdfAnnotationOverlay> &overlays);
    void formFieldOverlaysChanged(const QVector<PdfFormFieldOverlay> &overlays);
    void redactionOverlaysChanged(const QVector<PdfRedactionOverlay> &overlays);
    void textSelectionChanged(bool hasSelection);
    void overlaySelectionChanged(bool hasSelection);
    void searchStateChanged(bool hasResults, int currentHit, int totalHits, const QString &statusText);
    void errorOccurred(const QString &message);
    void statusMessageChanged(const QString &message);
    void busyStateChanged(bool busy, const QString &message);
    void ocrFinished(const QString &title, const QString &text);
    void historyStateChanged(bool canUndo, bool canRedo);

private:
    void startOcrJob(const QImage &image, const QString &busyMessage, const QString &resultTitle, int pageSegmentationMode);
    QString annotationSidecarPath() const;
    void loadSidecarState();
    void saveSidecarState() const;
    QJsonObject currentDocumentState() const;
    void restoreDocumentState(const QJsonObject &state);
    void resetHistory();
    void recordHistorySnapshot();
    void updateHistoryState();
    void resetDocumentState();
    void clearSelectionInternal(bool emitSignal);
    QRectF imageRectToPageRect(const QRectF &imageRect) const;
    QPointF imagePointToPagePoint(const QPointF &imagePoint) const;
    QVector<QRectF> pageRectsToImageRects(const QVector<QRectF> &pageRects) const;
    QRectF pageRectToImageRect(const QRectF &pageRect) const;
    void emitOverlayState();
    void emitSearchState(const QString &fallbackMessage = QString());
    void updateOverlaySelectionSignal();
    void renderCurrentPage();

    std::unique_ptr<PdfRenderEngine> m_renderEngine;
    QString m_documentPath;
    QStringList m_pageLabels;
    QVector<PdfOutlineEntry> m_outlineEntries;
    PdfDocumentMetadata m_metadata;
    SelectionModel m_selectionModel;
    SearchModel m_searchModel;
    AnnotationModel m_annotationModel;
    FormFieldModel m_formFieldModel;
    RedactionModel m_redactionModel;
    QVector<PdfFormField> m_baseFormFields;
    std::unique_ptr<PageThumbnailProvider> m_thumbnailProvider;
    QImage m_currentPageImage;
    QString m_lastError;
    QSizeF m_pageSizePoints;
    QRectF m_lastSelectionPageRect;
    int m_currentPageIndex {0};
    double m_zoomFactor {1.0};
    QByteArray m_currentOwnerPassword;
    QByteArray m_currentUserPassword;
    QVector<QJsonObject> m_historySnapshots;
    int m_historyIndex {-1};
    QFutureWatcher<OcrJobResult> *m_ocrWatcher {nullptr};
};
