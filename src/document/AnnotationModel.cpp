#include "document/AnnotationModel.h"

#include <QUuid>

#include <QJsonObject>

namespace
{
QString generateAnnotationId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

PdfTextStyle normalizedTextStyle(const PdfTextStyle &style)
{
    PdfTextStyle normalized = style;
    if (!normalized.textColor.isValid()) {
        normalized.textColor = Qt::black;
    }

    const QString family = normalized.fontFamily.trimmed();
    if (family.contains(QStringLiteral("courier"), Qt::CaseInsensitive)) {
        normalized.fontFamily = QStringLiteral("Courier");
    } else if (family.contains(QStringLiteral("times"), Qt::CaseInsensitive)) {
        normalized.fontFamily = QStringLiteral("Times New Roman");
    } else {
        normalized.fontFamily = QStringLiteral("Helvetica");
    }

    if (normalized.fontSize <= 0.0) {
        normalized.fontSize = 12.0;
    }
    return normalized;
}
}

void AnnotationModel::clear()
{
    m_annotations.clear();
}

void AnnotationModel::setAnnotations(const QVector<PdfAnnotation> &annotations)
{
    m_annotations = annotations;
}

bool AnnotationModel::addHighlightFromSelection(const SelectionModel &selection)
{
    if (selection.isEmpty()) {
        return false;
    }

    PdfAnnotation annotation;
    annotation.id = generateAnnotationId();
    annotation.kind = PdfAnnotationKind::Highlight;
    annotation.pageIndex = selection.pageIndex();
    annotation.pageRects = selection.pageRects();
    annotation.text = selection.plainText();

    if (annotation.pageIndex < 0 || annotation.pageRects.isEmpty()) {
        return false;
    }

    clearSelection();
    annotation.selected = true;
    m_annotations.append(annotation);
    return true;
}

bool AnnotationModel::addRectangle(int pageIndex, const QRectF &pageRect)
{
    if (pageIndex < 0 || pageRect.isEmpty()) {
        return false;
    }

    PdfAnnotation annotation;
    annotation.id = generateAnnotationId();
    annotation.kind = PdfAnnotationKind::Rectangle;
    annotation.pageIndex = pageIndex;
    annotation.pageRects = { pageRect.normalized() };
    annotation.color = QColor(25, 118, 210, 180);

    clearSelection();
    annotation.selected = true;
    m_annotations.append(annotation);
    return true;
}

bool AnnotationModel::addNote(int pageIndex, const QRectF &pageRect, const QString &text)
{
    if (pageIndex < 0 || pageRect.isEmpty() || text.trimmed().isEmpty()) {
        return false;
    }

    PdfAnnotation annotation;
    annotation.id = generateAnnotationId();
    annotation.kind = PdfAnnotationKind::Note;
    annotation.pageIndex = pageIndex;
    annotation.pageRects = { pageRect.normalized() };
    annotation.color = QColor(255, 193, 7, 210);
    annotation.text = text.trimmed();

    clearSelection();
    annotation.selected = true;
    m_annotations.append(annotation);
    return true;
}

bool AnnotationModel::addFreeText(
    int pageIndex,
    const QRectF &pageRect,
    const QString &text,
    const PdfTextStyle &style,
    const QColor &backgroundColor)
{
    if (pageIndex < 0 || pageRect.isEmpty() || text.trimmed().isEmpty()) {
        return false;
    }

    PdfAnnotation annotation;
    annotation.id = generateAnnotationId();
    annotation.kind = PdfAnnotationKind::FreeText;
    annotation.pageIndex = pageIndex;
    annotation.pageRects = { pageRect.normalized() };
    annotation.color = backgroundColor.isValid() ? backgroundColor : QColor(255, 255, 255, 230);
    annotation.textStyle = normalizedTextStyle(style);
    annotation.text = text.trimmed();

    clearSelection();
    annotation.selected = true;
    m_annotations.append(annotation);
    return true;
}

bool AnnotationModel::addSignature(int pageIndex, const QRectF &pageRect, const QByteArray &imageBytes)
{
    if (pageIndex < 0 || pageRect.isEmpty() || imageBytes.isEmpty()) {
        return false;
    }

    PdfAnnotation annotation;
    annotation.id = generateAnnotationId();
    annotation.kind = PdfAnnotationKind::Signature;
    annotation.pageIndex = pageIndex;
    annotation.pageRects = { pageRect.normalized() };
    annotation.binaryPayload = imageBytes;

    clearSelection();
    annotation.selected = true;
    m_annotations.append(annotation);
    return true;
}

bool AnnotationModel::remove(const QString &annotationId)
{
    for (qsizetype index = 0; index < m_annotations.size(); ++index) {
        if (m_annotations.at(index).id == annotationId) {
            m_annotations.removeAt(index);
            return true;
        }
    }

    return false;
}

bool AnnotationModel::setColor(const QString &annotationId, const QColor &color)
{
    if (!color.isValid()) {
        return false;
    }

    if (PdfAnnotation *annotation = findMutable(annotationId)) {
        annotation->color = color;
        return true;
    }

    return false;
}

bool AnnotationModel::setText(const QString &annotationId, const QString &text)
{
    if (PdfAnnotation *annotation = findMutable(annotationId)) {
        if (annotation->kind != PdfAnnotationKind::Note && annotation->kind != PdfAnnotationKind::FreeText) {
            return false;
        }

        const QString trimmedText = text.trimmed();
        if (trimmedText.isEmpty()) {
            return false;
        }

        annotation->text = trimmedText;
        return true;
    }

    return false;
}

bool AnnotationModel::setFreeTextStyle(const QString &annotationId, const PdfTextStyle &style)
{
    if (PdfAnnotation *annotation = findMutable(annotationId)) {
        if (annotation->kind != PdfAnnotationKind::FreeText) {
            return false;
        }

        annotation->textStyle = normalizedTextStyle(style);
        return true;
    }

    return false;
}

bool AnnotationModel::translateSelected(const QPointF &pageDelta)
{
    if (pageDelta.isNull()) {
        return false;
    }

    for (PdfAnnotation &annotation : m_annotations) {
        if (!annotation.selected) {
            continue;
        }
        if (annotation.kind != PdfAnnotationKind::Signature
            && annotation.kind != PdfAnnotationKind::FreeText) {
            return false;
        }

        for (QRectF &rect : annotation.pageRects) {
            rect.translate(pageDelta);
        }
        return true;
    }

    return false;
}

bool AnnotationModel::resizeSelectedFreeText(const QSizeF &pageDelta, const QSizeF &minimumSize)
{
    for (PdfAnnotation &annotation : m_annotations) {
        if (!annotation.selected) {
            continue;
        }
        if (annotation.kind != PdfAnnotationKind::FreeText || annotation.pageRects.isEmpty()) {
            return false;
        }

        QRectF &rect = annotation.pageRects[0];
        rect.setSize(QSizeF(qMax(minimumSize.width(), rect.width() + pageDelta.width()),
                            qMax(minimumSize.height(), rect.height() + pageDelta.height())));
        return true;
    }

    return false;
}

bool AnnotationModel::remapPages(const QVector<int> &newOrder)
{
    QVector<PdfAnnotation> remappedAnnotations;
    remappedAnnotations.reserve(m_annotations.size());

    for (const PdfAnnotation &annotation : std::as_const(m_annotations)) {
        const int newPageIndex = newOrder.indexOf(annotation.pageIndex);
        if (newPageIndex < 0) {
            continue;
        }

        PdfAnnotation remapped = annotation;
        remapped.pageIndex = newPageIndex;
        remappedAnnotations.append(remapped);
    }

    m_annotations = remappedAnnotations;
    return true;
}

void AnnotationModel::clearSelection()
{
    for (PdfAnnotation &annotation : m_annotations) {
        annotation.selected = false;
    }
}

bool AnnotationModel::selectAt(int pageIndex, const QPointF &pagePoint)
{
    clearSelection();

    for (qsizetype index = m_annotations.size() - 1; index >= 0; --index) {
        PdfAnnotation &annotation = m_annotations[index];
        if (annotation.pageIndex == pageIndex && hitTest(annotation, pagePoint)) {
            annotation.selected = true;
            return true;
        }
    }

    return false;
}

QString AnnotationModel::selectedAnnotationId() const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.selected) {
            return annotation.id;
        }
    }

    return {};
}

bool AnnotationModel::hasSelectedAnnotation() const
{
    return !selectedAnnotationId().isEmpty();
}

bool AnnotationModel::hasAnnotationKind(PdfAnnotationKind kind) const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.kind == kind) {
            return true;
        }
    }

    return false;
}

PdfAnnotationKind AnnotationModel::selectedAnnotationKind() const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.selected) {
            return annotation.kind;
        }
    }

    return PdfAnnotationKind::Highlight;
}

QColor AnnotationModel::selectedAnnotationColor() const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.selected) {
            return annotation.color;
        }
    }

    return {};
}

QString AnnotationModel::selectedAnnotationText() const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.selected) {
            return annotation.text;
        }
    }

    return {};
}

PdfTextStyle AnnotationModel::selectedAnnotationTextStyle() const
{
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.selected) {
            return annotation.textStyle;
        }
    }

    return {};
}

QVector<PdfAnnotation> AnnotationModel::annotations() const
{
    return m_annotations;
}

QVector<PdfAnnotation> AnnotationModel::annotationsForPage(int pageIndex) const
{
    QVector<PdfAnnotation> annotations;
    for (const PdfAnnotation &annotation : m_annotations) {
        if (annotation.pageIndex == pageIndex) {
            annotations.append(annotation);
        }
    }
    return annotations;
}

QJsonArray AnnotationModel::toJson() const
{
    QJsonArray annotationsArray;

    for (const PdfAnnotation &annotation : m_annotations) {
        QJsonObject annotationObject;
        annotationObject.insert(QStringLiteral("id"), annotation.id);
        annotationObject.insert(QStringLiteral("kind"), static_cast<int>(annotation.kind));
        annotationObject.insert(QStringLiteral("pageIndex"), annotation.pageIndex);
        annotationObject.insert(QStringLiteral("text"), annotation.text);
        annotationObject.insert(QStringLiteral("color"), annotation.color.name(QColor::HexArgb));
        annotationObject.insert(QStringLiteral("textColor"), annotation.textStyle.textColor.name(QColor::HexArgb));
        annotationObject.insert(QStringLiteral("fontFamily"), annotation.textStyle.fontFamily);
        annotationObject.insert(QStringLiteral("fontSize"), annotation.textStyle.fontSize);
        if (!annotation.binaryPayload.isEmpty()) {
            annotationObject.insert(QStringLiteral("binaryPayload"), QString::fromLatin1(annotation.binaryPayload.toBase64()));
        }

        QJsonArray rectsArray;
        for (const QRectF &rect : annotation.pageRects) {
            QJsonObject rectObject;
            rectObject.insert(QStringLiteral("x"), rect.x());
            rectObject.insert(QStringLiteral("y"), rect.y());
            rectObject.insert(QStringLiteral("width"), rect.width());
            rectObject.insert(QStringLiteral("height"), rect.height());
            rectsArray.append(rectObject);
        }

        annotationObject.insert(QStringLiteral("rects"), rectsArray);
        annotationsArray.append(annotationObject);
    }

    return annotationsArray;
}

bool AnnotationModel::fromJson(const QJsonArray &annotationsArray)
{
    clear();

    for (const QJsonValue &value : annotationsArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject annotationObject = value.toObject();
        PdfAnnotation annotation;
        annotation.id = annotationObject.value(QStringLiteral("id")).toString(generateAnnotationId());
        annotation.kind = static_cast<PdfAnnotationKind>(annotationObject.value(QStringLiteral("kind")).toInt(0));
        annotation.pageIndex = annotationObject.value(QStringLiteral("pageIndex")).toInt(-1);
        annotation.text = annotationObject.value(QStringLiteral("text")).toString();
        annotation.color = QColor(annotationObject.value(QStringLiteral("color")).toString(QStringLiteral("#6EFFEB3B")));
        annotation.textStyle.textColor = QColor(annotationObject.value(QStringLiteral("textColor")).toString(QStringLiteral("#ff000000")));
        annotation.textStyle.fontFamily = annotationObject.value(QStringLiteral("fontFamily")).toString(QStringLiteral("Helvetica"));
        annotation.textStyle.fontSize = annotationObject.value(QStringLiteral("fontSize")).toDouble(12.0);
        annotation.binaryPayload = QByteArray::fromBase64(
            annotationObject.value(QStringLiteral("binaryPayload")).toString().toLatin1());

        const QJsonArray rectsArray = annotationObject.value(QStringLiteral("rects")).toArray();
        for (const QJsonValue &rectValue : rectsArray) {
            const QJsonObject rectObject = rectValue.toObject();
            annotation.pageRects.append(QRectF(rectObject.value(QStringLiteral("x")).toDouble(),
                                               rectObject.value(QStringLiteral("y")).toDouble(),
                                               rectObject.value(QStringLiteral("width")).toDouble(),
                                               rectObject.value(QStringLiteral("height")).toDouble()));
        }

        if (annotation.pageIndex >= 0 && !annotation.pageRects.isEmpty()) {
            annotation.textStyle = normalizedTextStyle(annotation.textStyle);
            m_annotations.append(annotation);
        }
    }

    return true;
}

PdfAnnotation *AnnotationModel::findMutable(const QString &annotationId)
{
    for (PdfAnnotation &annotation : m_annotations) {
        if (annotation.id == annotationId) {
            return &annotation;
        }
    }

    return nullptr;
}

bool AnnotationModel::hitTest(const PdfAnnotation &annotation, const QPointF &pagePoint) const
{
    for (const QRectF &rect : annotation.pageRects) {
        const QRectF expandedRect = annotation.kind == PdfAnnotationKind::Note
            ? rect.adjusted(-8.0, -8.0, 8.0, 8.0)
            : rect;
        if (expandedRect.contains(pagePoint)) {
            return true;
        }
    }

    return false;
}
