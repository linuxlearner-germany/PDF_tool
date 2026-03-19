#pragma once

#include <QColor>
#include <QRectF>
#include <QString>
#include <QVector>

struct PdfDocumentMetadata
{
    QString title;
    QString author;
    QString subject;
    QString creator;
    QString producer;
    QString filePath;
    int pageCount {0};
};

struct PdfOutlineEntry
{
    QString title;
    int pageIndex {-1};
    bool initiallyOpen {false};
    QVector<PdfOutlineEntry> children;
};

struct PdfSearchHit
{
    int pageIndex {-1};
    QRectF pageRect;
    QString previewText;
};

enum class PdfAnnotationKind
{
    Highlight,
    Rectangle,
    Note,
    FreeText
};

struct PdfAnnotation
{
    QString id;
    PdfAnnotationKind kind {PdfAnnotationKind::Highlight};
    int pageIndex {-1};
    QVector<QRectF> pageRects;
    QColor color {255, 235, 59, 110};
    QString text;
    bool selected {false};
};

struct PdfAnnotationOverlay
{
    QString id;
    PdfAnnotationKind kind {PdfAnnotationKind::Highlight};
    QVector<QRectF> imageRects;
    QColor color;
    QString text;
    bool selected {false};
};

enum class PdfFormFieldKind
{
    Text,
    CheckBox
};

struct PdfFormField
{
    QString id;
    int pageIndex {-1};
    PdfFormFieldKind kind {PdfFormFieldKind::Text};
    QRectF pageRect;
    QString name;
    QString label;
    QString textValue;
    bool checked {false};
    bool readOnly {false};
};

struct PdfFormFieldOverlay
{
    QString id;
    PdfFormFieldKind kind {PdfFormFieldKind::Text};
    QRectF imageRect;
    QString label;
    QString textValue;
    bool checked {false};
    bool readOnly {false};
};

struct PdfRedaction
{
    QString id;
    int pageIndex {-1};
    QRectF pageRect;
    QString reason;
    bool selected {false};
};

struct PdfRedactionOverlay
{
    QString id;
    QRectF imageRect;
    bool selected {false};
};
