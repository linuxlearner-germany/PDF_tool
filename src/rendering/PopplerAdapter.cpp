#include "rendering/PopplerAdapter.h"

#include <algorithm>
#include <memory>

#include <QBuffer>
#include <QFileInfo>

#include <poppler-annotation.h>
#include <poppler-form.h>
#include <poppler-link.h>
#include <poppler-qt6.h>

namespace
{
constexpr double kBaseDpi = 72.0;
constexpr double kAnnotationPreviewDpi = 144.0;

QByteArray renderAnnotationAppearancePng(
    Poppler::Page &page,
    const QRectF &normalizedBoundary,
    const QSizeF &pageSize)
{
    if (!normalizedBoundary.isValid() || normalizedBoundary.isEmpty() || pageSize.isEmpty()) {
        return {};
    }

    const QRectF pageRect(normalizedBoundary.x() * pageSize.width(),
                          normalizedBoundary.y() * pageSize.height(),
                          normalizedBoundary.width() * pageSize.width(),
                          normalizedBoundary.height() * pageSize.height());
    const double scale = kAnnotationPreviewDpi / kBaseDpi;
    const int x = qMax(0, static_cast<int>(std::floor(pageRect.left() * scale)));
    const int y = qMax(0, static_cast<int>(std::floor(pageRect.top() * scale)));
    const int width = qMax(1, static_cast<int>(std::ceil(pageRect.width() * scale)));
    const int height = qMax(1, static_cast<int>(std::ceil(pageRect.height() * scale)));

    const QImage image = page.renderToImage(kAnnotationPreviewDpi, kAnnotationPreviewDpi, x, y, width, height);
    if (image.isNull()) {
        return {};
    }

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) {
        return {};
    }

    return pngBytes;
}
}

PopplerAdapter::PopplerAdapter() = default;

PopplerAdapter::~PopplerAdapter() = default;

bool PopplerAdapter::loadDocument(const QString &filePath, const QByteArray &ownerPassword, const QByteArray &userPassword)
{
    closeDocument();

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_lastError = QStringLiteral("Datei nicht gefunden.");
        return false;
    }

    std::unique_ptr<Poppler::Document> document(Poppler::Document::load(filePath, ownerPassword, userPassword));
    if (!document) {
        m_lastError = QStringLiteral("PDF konnte nicht geladen werden.");
        return false;
    }

    if (document->isLocked()) {
        m_requiresPassword = true;
        m_lastError = userPassword.isEmpty()
            ? QStringLiteral("PDF ist passwortgeschützt.")
            : QStringLiteral("Passwort ist falsch oder unzureichend.");
        return false;
    }

    document->setRenderHint(Poppler::Document::TextAntialiasing, true);
    document->setRenderHint(Poppler::Document::Antialiasing, true);

    m_document = std::move(document);
    m_requiresPassword = false;
    m_lastError.clear();
    return true;
}

void PopplerAdapter::closeDocument()
{
    m_document.reset();
    m_lastError.clear();
    m_requiresPassword = false;
}

bool PopplerAdapter::isLoaded() const
{
    return static_cast<bool>(m_document);
}

bool PopplerAdapter::requiresPassword() const
{
    return m_requiresPassword;
}

std::unique_ptr<Poppler::Page> PopplerAdapter::loadPage(int pageIndex)
{
    if (!m_document) {
        m_lastError = QStringLiteral("Kein PDF geladen.");
        return {};
    }

    if (pageIndex < 0 || pageIndex >= m_document->numPages()) {
        m_lastError = QStringLiteral("Seite außerhalb des gültigen Bereichs.");
        return {};
    }

    std::unique_ptr<Poppler::Page> page(m_document->page(pageIndex));
    if (!page) {
        m_lastError = QStringLiteral("Seite konnte nicht geöffnet werden.");
        return {};
    }

    return page;
}

PdfOutlineEntry PopplerAdapter::convertOutlineItem(const Poppler::OutlineItem &item) const
{
    PdfOutlineEntry entry;
    entry.title = item.name();
    entry.initiallyOpen = item.isOpen();

    if (const QSharedPointer<const Poppler::LinkDestination> destination = item.destination()) {
        entry.pageIndex = destination->pageNumber() > 0 ? destination->pageNumber() - 1 : -1;
    }

    if (item.hasChildren()) {
        const QVector<Poppler::OutlineItem> children = item.children();
        entry.children.reserve(children.size());
        for (const Poppler::OutlineItem &child : children) {
            if (!child.isNull()) {
                entry.children.append(convertOutlineItem(child));
            }
        }
    }

    return entry;
}

int PopplerAdapter::pageCount() const
{
    return m_document ? m_document->numPages() : 0;
}

QImage PopplerAdapter::renderPage(int pageIndex, double scale)
{
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return {};
    }

    const double safeScale = std::max(0.1, scale);
    const double dpi = kBaseDpi * safeScale;
    const QImage image = page->renderToImage(dpi, dpi);

    if (image.isNull()) {
        m_lastError = QStringLiteral("Seite konnte nicht gerendert werden.");
        return {};
    }

    m_lastError.clear();
    return image;
}

QSizeF PopplerAdapter::pageSizePoints(int pageIndex)
{
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return {};
    }

    m_lastError.clear();
    return page->pageSizeF();
}

QString PopplerAdapter::pageLabel(int pageIndex)
{
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return QString::number(pageIndex + 1);
    }

    const QString label = page->label();
    m_lastError.clear();
    return label.isEmpty() ? QString::number(pageIndex + 1) : label;
}

QStringList PopplerAdapter::pageLabels()
{
    QStringList labels;
    labels.reserve(pageCount());
    for (int pageIndex = 0; pageIndex < pageCount(); ++pageIndex) {
        labels.append(pageLabel(pageIndex));
    }
    return labels;
}

PdfTextSelection PopplerAdapter::buildTextSelection(int pageIndex, const QRectF &pageRect)
{
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return {};
    }

    PdfTextSelection selection;
    const QRectF normalizedRect = pageRect.normalized();
    const auto textBoxes = page->textList();
    selection.fragments.reserve(static_cast<qsizetype>(textBoxes.size()));

    for (const auto &textBox : textBoxes) {
        if (!textBox) {
            continue;
        }

        const QRectF textBounds = textBox->boundingBox();
        if (normalizedRect.intersects(textBounds)) {
            selection.fragments.append(PdfTextSelectionFragment {
                textBounds,
                textBox->text(),
                textBox->hasSpaceAfter()
            });
        }
    }

    m_lastError.clear();
    return selection;
}

QVector<PdfSearchHit> PopplerAdapter::search(const QString &query)
{
    QVector<PdfSearchHit> hits;
    if (!m_document || query.trimmed().isEmpty()) {
        return hits;
    }

    for (int pageIndex = 0; pageIndex < m_document->numPages(); ++pageIndex) {
        std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
        if (!page) {
            continue;
        }

        const QList<QRectF> rects = page->search(query, Poppler::Page::IgnoreCase);
        for (const QRectF &rect : rects) {
            PdfSearchHit hit;
            hit.pageIndex = pageIndex;
            hit.pageRect = rect;
            hit.previewText = page->text(rect.adjusted(-24.0, -8.0, 24.0, 8.0)).simplified();
            hits.append(hit);
        }
    }

    m_lastError.clear();
    return hits;
}

QVector<PdfAnnotation> PopplerAdapter::annotations(int pageIndex)
{
    QVector<PdfAnnotation> result;
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return result;
    }

    const QSizeF pageSize = page->pageSizeF();
    const std::vector<std::unique_ptr<Poppler::Annotation>> pageAnnotations = page->annotations();
    result.reserve(static_cast<qsizetype>(pageAnnotations.size()));

    auto toPageRect = [&pageSize](const QRectF &normalizedRect) {
        return QRectF(
            normalizedRect.x() * pageSize.width(),
            normalizedRect.y() * pageSize.height(),
            normalizedRect.width() * pageSize.width(),
            normalizedRect.height() * pageSize.height()).normalized();
    };

    for (const std::unique_ptr<Poppler::Annotation> &annotation : pageAnnotations) {
        if (!annotation) {
            continue;
        }

        PdfAnnotation nativeAnnotation;
        nativeAnnotation.id = annotation->uniqueName();
        nativeAnnotation.pageIndex = pageIndex;
        nativeAnnotation.text = annotation->contents();
        nativeAnnotation.color = annotation->style().color();
        if (!nativeAnnotation.color.isValid()) {
            nativeAnnotation.color = QColor(255, 235, 59, 110);
        }

        switch (annotation->subType()) {
        case Poppler::Annotation::AHighlight: {
            const auto *highlight = dynamic_cast<Poppler::HighlightAnnotation *>(annotation.get());
            if (!highlight || highlight->highlightType() != Poppler::HighlightAnnotation::Highlight) {
                continue;
            }
            nativeAnnotation.kind = PdfAnnotationKind::Highlight;
            const QList<Poppler::HighlightAnnotation::Quad> quads = highlight->highlightQuads();
            for (const auto &quad : quads) {
                QRectF quadRect;
                for (const QPointF &point : quad.points) {
                    quadRect = quadRect.isNull() ? QRectF(point, QSizeF()) : quadRect.united(QRectF(point, QSizeF()));
                }
                if (!quadRect.isNull()) {
                    nativeAnnotation.pageRects.append(toPageRect(quadRect.normalized()));
                }
            }
            break;
        }
        case Poppler::Annotation::AGeom: {
            const auto *geom = dynamic_cast<Poppler::GeomAnnotation *>(annotation.get());
            if (!geom || geom->geomType() != Poppler::GeomAnnotation::InscribedSquare) {
                continue;
            }
            nativeAnnotation.kind = PdfAnnotationKind::Rectangle;
            nativeAnnotation.pageRects.append(toPageRect(annotation->boundary()));
            break;
        }
        case Poppler::Annotation::AText: {
            const auto *textAnnotation = dynamic_cast<Poppler::TextAnnotation *>(annotation.get());
            if (!textAnnotation) {
                continue;
            }
            nativeAnnotation.kind = textAnnotation->textType() == Poppler::TextAnnotation::Linked
                ? PdfAnnotationKind::Note
                : PdfAnnotationKind::FreeText;
            nativeAnnotation.pageRects.append(toPageRect(annotation->boundary()));
            break;
        }
        case Poppler::Annotation::AStamp:
            nativeAnnotation.kind = PdfAnnotationKind::Signature;
            nativeAnnotation.pageRects.append(toPageRect(annotation->boundary()));
            nativeAnnotation.binaryPayload =
                renderAnnotationAppearancePng(*page, annotation->boundary(), pageSize);
            break;
        default:
            continue;
        }

        if (!nativeAnnotation.pageRects.isEmpty()) {
            result.append(nativeAnnotation);
        }
    }

    m_lastError.clear();
    return result;
}

QVector<PdfFormField> PopplerAdapter::formFields(int pageIndex)
{
    QVector<PdfFormField> fields;
    std::unique_ptr<Poppler::Page> page = loadPage(pageIndex);
    if (!page) {
        return fields;
    }

    const QSizeF pageSize = page->pageSizeF();
    const std::vector<std::unique_ptr<Poppler::FormField>> popplerFields = page->formFields();
    fields.reserve(static_cast<qsizetype>(popplerFields.size()));

    for (const std::unique_ptr<Poppler::FormField> &field : popplerFields) {
        if (!field || !field->isVisible()) {
            continue;
        }

        PdfFormField formField;
        formField.id = QStringLiteral("page_%1_field_%2").arg(pageIndex).arg(field->id());
        formField.pageIndex = pageIndex;
        formField.name = field->fullyQualifiedName();
        formField.label = field->uiName().isEmpty() ? field->name() : field->uiName();
        formField.textStyle.fontFamily = QStringLiteral("Helvetica");
        formField.textStyle.fontSize = 12.0;
        formField.textStyle.textColor = Qt::black;
        formField.readOnly = field->isReadOnly();

        const QRectF normalizedRect = field->rect();
        formField.pageRect = QRectF(normalizedRect.x() * pageSize.width(),
                                    normalizedRect.y() * pageSize.height(),
                                    normalizedRect.width() * pageSize.width(),
                                    normalizedRect.height() * pageSize.height()).normalized();

        switch (field->type()) {
        case Poppler::FormField::FormText: {
            const auto *textField = dynamic_cast<Poppler::FormFieldText *>(field.get());
            if (!textField) {
                continue;
            }
            formField.kind = PdfFormFieldKind::Text;
            formField.textValue = textField->text();
            fields.append(formField);
            break;
        }
        case Poppler::FormField::FormButton: {
            const auto *buttonField = dynamic_cast<Poppler::FormFieldButton *>(field.get());
            if (!buttonField || buttonField->buttonType() != Poppler::FormFieldButton::CheckBox) {
                continue;
            }
            formField.kind = PdfFormFieldKind::CheckBox;
            formField.checked = buttonField->state();
            fields.append(formField);
            break;
        }
        default:
            break;
        }
    }

    m_lastError.clear();
    return fields;
}

PdfDocumentMetadata PopplerAdapter::documentMetadata() const
{
    PdfDocumentMetadata metadata;
    if (!m_document) {
        return metadata;
    }

    metadata.title = m_document->title();
    metadata.author = m_document->author();
    metadata.subject = m_document->info(QStringLiteral("Subject"));
    metadata.creator = m_document->info(QStringLiteral("Creator"));
    metadata.producer = m_document->info(QStringLiteral("Producer"));
    metadata.pageCount = m_document->numPages();
    return metadata;
}

QVector<PdfOutlineEntry> PopplerAdapter::outlineEntries() const
{
    QVector<PdfOutlineEntry> entries;
    if (!m_document) {
        return entries;
    }

    const QVector<Poppler::OutlineItem> outline = m_document->outline();
    entries.reserve(outline.size());
    for (const Poppler::OutlineItem &item : outline) {
        if (!item.isNull()) {
            entries.append(convertOutlineItem(item));
        }
    }

    return entries;
}

QString PopplerAdapter::lastError() const
{
    return m_lastError;
}
