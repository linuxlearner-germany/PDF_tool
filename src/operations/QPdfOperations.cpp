#include "operations/QPdfOperations.h"

#include <algorithm>
#include <map>

#include <QDir>
#include <QFileInfo>
#include <QImage>

#ifdef PDF_TOOL_HAS_QPDF
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFAcroFormDocumentHelper.hh>
#include <qpdf/QPDFFormFieldObjectHelper.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFExc.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFWriter.hh>
#endif

bool QPdfOperations::isAvailable() const
{
#ifdef PDF_TOOL_HAS_QPDF
    return true;
#else
    return false;
#endif
}

QString QPdfOperations::backendName() const
{
    return QStringLiteral("qpdf");
}

QString QPdfOperations::availabilityError() const
{
#ifdef PDF_TOOL_HAS_QPDF
    return {};
#else
    return QStringLiteral("qpdf wurde beim Build nicht gefunden. Native PDF-Operationen sind deaktiviert.");
#endif
}

CapabilityState QPdfOperations::capabilityState() const
{
    CapabilityState state;
    state.available = isAvailable();
    if (!state.available) {
        state.error = availabilityError();
    }
    return state;
}

QString QPdfOperations::lastError() const
{
    return m_lastError;
}

namespace
{
#ifdef PDF_TOOL_HAS_QPDF
std::string toUtf8(const QString &text)
{
    return text.toUtf8().toStdString();
}

QString standardPdfFontAlias(const QString &fontFamily)
{
    const QString family = fontFamily.trimmed().toLower();
    if (family.contains(QStringLiteral("courier"))) {
        return QStringLiteral("/Cour");
    }
    if (family.contains(QStringLiteral("times"))) {
        return QStringLiteral("/TiRo");
    }
    return QStringLiteral("/Helv");
}

QString normalizedPdfFontFamily(const QString &fontFamily)
{
    const QString family = fontFamily.trimmed();
    if (family.contains(QStringLiteral("courier"), Qt::CaseInsensitive)) {
        return QStringLiteral("Courier");
    }
    if (family.contains(QStringLiteral("times"), Qt::CaseInsensitive)) {
        return QStringLiteral("Times New Roman");
    }
    return QStringLiteral("Helvetica");
}

QString defaultAppearanceString(const PdfAnnotation &annotation)
{
    const QColor textColor = annotation.textStyle.textColor.isValid() ? annotation.textStyle.textColor.toRgb() : QColor(Qt::black);
    const double fontSize = annotation.textStyle.fontSize > 0.0 ? annotation.textStyle.fontSize : 12.0;
    const QString fontAlias = standardPdfFontAlias(annotation.textStyle.fontFamily);
    return QStringLiteral("%1 %2 %3 rg %4 %5 Tf")
        .arg(QString::number(textColor.redF(), 'f', 3))
        .arg(QString::number(textColor.greenF(), 'f', 3))
        .arg(QString::number(textColor.blueF(), 'f', 3))
        .arg(fontAlias)
        .arg(QString::number(fontSize, 'f', 1));
}

QString defaultAppearanceString(const PdfTextStyle &style)
{
    const QColor textColor = style.textColor.isValid() ? style.textColor.toRgb() : QColor(Qt::black);
    const double fontSize = style.fontSize > 0.0 ? style.fontSize : 12.0;
    const QString fontAlias = standardPdfFontAlias(style.fontFamily);
    return QStringLiteral("%1 %2 %3 rg %4 %5 Tf")
        .arg(QString::number(textColor.redF(), 'f', 3))
        .arg(QString::number(textColor.greenF(), 'f', 3))
        .arg(QString::number(textColor.blueF(), 'f', 3))
        .arg(fontAlias)
        .arg(QString::number(fontSize, 'f', 1));
}

QString defaultStyleString(const PdfAnnotation &annotation)
{
    const QColor textColor = annotation.textStyle.textColor.isValid() ? annotation.textStyle.textColor.toRgb() : QColor(Qt::black);
    const QColor backgroundColor = annotation.color.isValid() ? annotation.color.toRgb() : QColor(Qt::white);
    const QString family = normalizedPdfFontFamily(annotation.textStyle.fontFamily);
    const double fontSize = annotation.textStyle.fontSize > 0.0 ? annotation.textStyle.fontSize : 12.0;

    return QStringLiteral("font: %1pt '%2'; color: %3; background-color: %4;")
        .arg(QString::number(fontSize, 'f', 1))
        .arg(family)
        .arg(textColor.name(QColor::HexRgb))
        .arg(backgroundColor.name(QColor::HexRgb));
}

QPDFObjectHandle pdfReal(double value)
{
    return QPDFObjectHandle::newReal(value, 3, true);
}

QPDFObjectHandle pdfRect(double left, double bottom, double right, double top)
{
    return QPDFObjectHandle::newArray({
        pdfReal(left),
        pdfReal(bottom),
        pdfReal(right),
        pdfReal(top),
    });
}

QPDFObjectHandle pdfColor(const QColor &color)
{
    const QColor rgb = color.toRgb();
    return QPDFObjectHandle::newArray({
        pdfReal(rgb.redF()),
        pdfReal(rgb.greenF()),
        pdfReal(rgb.blueF()),
    });
}

QPDFObjectHandle createMarkupAnnotation(QPDF &pdf, const PdfAnnotation &annotation, const QSizeF &pageSize)
{
    if (annotation.pageRects.isEmpty() || !pageSize.isValid() || pageSize.isEmpty()) {
        return QPDFObjectHandle::newNull();
    }

    QRectF boundingRect;
    for (const QRectF &pageRect : annotation.pageRects) {
        const QRectF normalized = pageRect.normalized();
        boundingRect = boundingRect.isNull() ? normalized : boundingRect.united(normalized);
    }

    const double left = boundingRect.left();
    const double right = boundingRect.right();
    const double top = boundingRect.top();
    const double bottom = boundingRect.bottom();

    QPDFObjectHandle annot = QPDFObjectHandle::newDictionary();
    annot.replaceKey("/Type", QPDFObjectHandle::newName("/Annot"));
    annot.replaceKey("/NM", QPDFObjectHandle::newUnicodeString(toUtf8(annotation.id)));
    annot.replaceKey("/Rect", pdfRect(left, pageSize.height() - bottom, right, pageSize.height() - top));
    annot.replaceKey("/F", QPDFObjectHandle::newInteger(4));

    if (annotation.color.isValid()) {
        annot.replaceKey("/C", pdfColor(annotation.color));
    }

    switch (annotation.kind) {
    case PdfAnnotationKind::Highlight: {
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Highlight"));
        QPDFObjectHandle quadPoints = QPDFObjectHandle::newArray();
        for (const QRectF &pageRect : annotation.pageRects) {
            const QRectF normalized = pageRect.normalized();
            const double qLeft = normalized.left();
            const double qRight = normalized.right();
            const double qTop = pageSize.height() - normalized.top();
            const double qBottom = pageSize.height() - normalized.bottom();
            quadPoints.appendItem(pdfReal(qLeft));
            quadPoints.appendItem(pdfReal(qTop));
            quadPoints.appendItem(pdfReal(qRight));
            quadPoints.appendItem(pdfReal(qTop));
            quadPoints.appendItem(pdfReal(qLeft));
            quadPoints.appendItem(pdfReal(qBottom));
            quadPoints.appendItem(pdfReal(qRight));
            quadPoints.appendItem(pdfReal(qBottom));
        }
        annot.replaceKey("/QuadPoints", quadPoints);
        if (!annotation.text.isEmpty()) {
            annot.replaceKey("/Contents", QPDFObjectHandle::newUnicodeString(toUtf8(annotation.text)));
        }
        break;
    }
    case PdfAnnotationKind::Rectangle:
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Square"));
        annot.replaceKey("/Border", QPDFObjectHandle::newArray({
            QPDFObjectHandle::newInteger(0),
            QPDFObjectHandle::newInteger(0),
            QPDFObjectHandle::newInteger(2),
        }));
        break;
    case PdfAnnotationKind::Note:
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Text"));
        annot.replaceKey("/Name", QPDFObjectHandle::newName("/Comment"));
        annot.replaceKey("/Contents", QPDFObjectHandle::newUnicodeString(toUtf8(annotation.text)));
        break;
    case PdfAnnotationKind::FreeText:
        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/FreeText"));
        annot.replaceKey("/Contents", QPDFObjectHandle::newUnicodeString(toUtf8(annotation.text)));
        annot.replaceKey("/DA", QPDFObjectHandle::newString(toUtf8(defaultAppearanceString(annotation))));
        annot.replaceKey("/DS", QPDFObjectHandle::newUnicodeString(toUtf8(defaultStyleString(annotation))));
        annot.replaceKey("/BS", QPDFObjectHandle::newDictionary({
            {"/Type", QPDFObjectHandle::newName("/Border")},
            {"/W", QPDFObjectHandle::newInteger(0)},
        }));
        annot.replaceKey("/Q", QPDFObjectHandle::newInteger(0));
        break;
    case PdfAnnotationKind::Signature: {
        if (annotation.binaryPayload.isEmpty()) {
            return QPDFObjectHandle::newNull();
        }

        QImage signatureImage;
        signatureImage.loadFromData(annotation.binaryPayload);
        if (signatureImage.isNull()) {
            return QPDFObjectHandle::newNull();
        }

        QImage rgbImage = signatureImage.convertToFormat(QImage::Format_RGB888);
        if (rgbImage.isNull()) {
            return QPDFObjectHandle::newNull();
        }

        QByteArray imageBytes;
        imageBytes.reserve(rgbImage.width() * rgbImage.height() * 3);
        for (int y = 0; y < rgbImage.height(); ++y) {
            imageBytes.append(reinterpret_cast<const char *>(rgbImage.constScanLine(y)), rgbImage.width() * 3);
        }

        QPDFObjectHandle image = pdf.newStream(imageBytes.toStdString());
        QPDFObjectHandle imageDict = image.getDict();
        imageDict.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
        imageDict.replaceKey("/Subtype", QPDFObjectHandle::newName("/Image"));
        imageDict.replaceKey("/Width", QPDFObjectHandle::newInteger(rgbImage.width()));
        imageDict.replaceKey("/Height", QPDFObjectHandle::newInteger(rgbImage.height()));
        imageDict.replaceKey("/ColorSpace", QPDFObjectHandle::newName("/DeviceRGB"));
        imageDict.replaceKey("/BitsPerComponent", QPDFObjectHandle::newInteger(8));

        const double width = qMax(1.0, boundingRect.width());
        const double height = qMax(1.0, boundingRect.height());
        const std::string appearanceContent =
            "q\n" + std::to_string(width) + " 0 0 " + std::to_string(height) + " 0 0 cm\n/Im1 Do\nQ\n";
        QPDFObjectHandle appearance = pdf.newStream(appearanceContent);
        QPDFObjectHandle appearanceDict = appearance.getDict();
        appearanceDict.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
        appearanceDict.replaceKey("/Subtype", QPDFObjectHandle::newName("/Form"));
        appearanceDict.replaceKey("/BBox", pdfRect(0.0, 0.0, width, height));
        QPDFObjectHandle resources = QPDFObjectHandle::newDictionary();
        QPDFObjectHandle xObject = QPDFObjectHandle::newDictionary();
        xObject.replaceKey("/Im1", image);
        resources.replaceKey("/XObject", xObject);
        appearanceDict.replaceKey("/Resources", resources);

        annot.replaceKey("/Subtype", QPDFObjectHandle::newName("/Stamp"));
        annot.replaceKey("/Name", QPDFObjectHandle::newName("/Approved"));
        annot.replaceKey(
            "/AP",
            QPDFObjectHandle::newDictionary({
                {"/N", appearance},
            }));
        break;
    }
    }

    return pdf.makeIndirectObject(annot);
}

bool applyFormValues(
    QPDF &pdf,
    const QVector<PdfFormField> &formFields,
    QString &lastError)
{
    if (formFields.isEmpty()) {
        return true;
    }

    QPDFAcroFormDocumentHelper formHelper(pdf);
    if (!formHelper.hasAcroForm()) {
        lastError = QStringLiteral("Das Dokument enthält keine AcroForm-Felder.");
        return false;
    }

    std::map<int, QVector<PdfFormField>> fieldsByPage;
    for (const PdfFormField &field : formFields) {
        if (field.pageIndex >= 0) {
            fieldsByPage[field.pageIndex].append(field);
        }
    }

    const auto pages = QPDFPageDocumentHelper(pdf).getAllPages();
    for (const auto &entry : fieldsByPage) {
        const int pageIndex = entry.first;
        if (pageIndex < 0 || pageIndex >= static_cast<int>(pages.size())) {
            continue;
        }

        const auto widgetAnnotations = formHelper.getWidgetAnnotationsForPage(pages.at(static_cast<std::size_t>(pageIndex)));
        for (const PdfFormField &fieldState : entry.second) {
            bool updated = false;
            for (const auto &annotation : widgetAnnotations) {
                QPDFFormFieldObjectHelper field = formHelper.getFieldForAnnotation(annotation);
                if (field.isNull()) {
                    continue;
                }

                const std::string qualifiedName = field.getFullyQualifiedName();
                if (QString::fromStdString(qualifiedName) != fieldState.name) {
                    continue;
                }

                if (fieldState.kind == PdfFormFieldKind::Text && field.isText()) {
                    field.setV(fieldState.textValue.toUtf8().toStdString(), false);
                    const std::string defaultAppearance = toUtf8(defaultAppearanceString(fieldState.textStyle));
                    field.getObjectHandle().replaceKey("/DA", QPDFObjectHandle::newString(defaultAppearance));
                    QPDFAnnotationObjectHelper mutableAnnotation(annotation.getObjectHandle());
                    mutableAnnotation.getObjectHandle().replaceKey("/DA", QPDFObjectHandle::newString(defaultAppearance));
                    field.generateAppearance(mutableAnnotation);
                    updated = true;
                    break;
                }

                if (fieldState.kind == PdfFormFieldKind::CheckBox && field.isCheckbox()) {
                    field.setV(QPDFObjectHandle::newName(fieldState.checked ? "/Yes" : "/Off"), false);
                    updated = true;
                    break;
                }
            }

            if (!updated && !fieldState.name.isEmpty()) {
                lastError = QStringLiteral("Formularfeld konnte nicht aktualisiert werden: %1").arg(fieldState.name);
                return false;
            }
        }
    }

    formHelper.setNeedAppearances(false);
    formHelper.generateAppearancesIfNeeded();
    return true;
}

bool isManagedAnnotationSubtype(const QPDFObjectHandle &annotationObject)
{
    if (!annotationObject.isDictionary()) {
        return false;
    }

    const std::string subtype = annotationObject.getKey("/Subtype").getName();
    return subtype == "/Highlight"
        || subtype == "/Square"
        || subtype == "/Text"
        || subtype == "/FreeText"
        || subtype == "/Stamp";
}

QPDFObjectHandle filteredExistingAnnotations(const QPDFObjectHandle &annots)
{
    QPDFObjectHandle filtered = QPDFObjectHandle::newArray();
    if (!annots.isArray()) {
        return filtered;
    }

    for (const QPDFObjectHandle &item : annots.getArrayAsVector()) {
        QPDFObjectHandle annotationObject = item;
        if (annotationObject.isIndirect()) {
            annotationObject = annotationObject.getDict();
        }

        if (isManagedAnnotationSubtype(annotationObject)) {
            continue;
        }

        filtered.appendItem(item);
    }

    return filtered;
}

bool saveAnnotationsAndFormsImpl(
    QString &lastError,
    const QString &inputFile,
    const QString &outputFile,
    const QVector<PdfAnnotation> &annotations,
    const QVector<PdfFormField> &formFields,
    const QVector<PdfRedaction> &redactions,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    QPDF pdf;
    pdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

    QPDFPageDocumentHelper pageHelper(pdf);
    const auto pages = pageHelper.getAllPages();
    if (pages.empty()) {
        lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
        return false;
    }

    if (pageSizesPoints.size() != static_cast<int>(pages.size())) {
        lastError = QStringLiteral("Seitengrößen stimmen nicht mit dem Dokument überein.");
        return false;
    }

    std::map<int, QVector<PdfAnnotation>> annotationsByPage;
    for (const PdfAnnotation &annotation : annotations) {
        if (annotation.pageIndex < 0 || annotation.pageIndex >= pageSizesPoints.size()) {
            continue;
        }
        annotationsByPage[annotation.pageIndex].append(annotation);
    }

    for (const auto &entry : annotationsByPage) {
        const int pageIndex = entry.first;
        const QVector<PdfAnnotation> &pageAnnotations = entry.second;
        QPDFPageObjectHelper page = pages.at(static_cast<std::size_t>(pageIndex));
        QPDFObjectHandle pageObject = page.getObjectHandle();
        QPDFObjectHandle annots = filteredExistingAnnotations(pageObject.getKey("/Annots"));
        pageObject.replaceKey("/Annots", annots);

        for (const PdfAnnotation &annotation : pageAnnotations) {
            QPDFObjectHandle annotObject = createMarkupAnnotation(pdf, annotation, pageSizesPoints.at(pageIndex));
            if (!annotObject.isNull()) {
                annots.appendItem(annotObject);
            }
        }
    }

    std::map<int, QVector<PdfRedaction>> redactionsByPage;
    for (const PdfRedaction &redaction : redactions) {
        if (redaction.pageIndex < 0 || redaction.pageIndex >= pageSizesPoints.size()) {
            continue;
        }
        redactionsByPage[redaction.pageIndex].append(redaction);
    }

    for (const auto &entry : redactionsByPage) {
        const int pageIndex = entry.first;
        const QVector<PdfRedaction> &pageRedactions = entry.second;
        QPDFPageObjectHelper page = pages.at(static_cast<std::size_t>(pageIndex));
        const QSizeF pageSize = pageSizesPoints.at(pageIndex);

        std::string content = "q\n0 0 0 rg\n";
        for (const PdfRedaction &redaction : pageRedactions) {
            const QRectF rect = redaction.pageRect.normalized();
            const double x = rect.left();
            const double y = pageSize.height() - rect.bottom();
            const double width = rect.width();
            const double height = rect.height();
            content += std::to_string(x) + " "
                + std::to_string(y) + " "
                + std::to_string(width) + " "
                + std::to_string(height) + " re f\n";
        }
        content += "Q\n";

        page.addPageContents(pdf.newStream(content), false);
    }

    if (!applyFormValues(pdf, formFields, lastError)) {
        return false;
    }

    QPDFWriter writer(pdf, outputFile.toLocal8Bit().constData());
    writer.write();
    return true;
}

bool saveDestructiveRedactedStateImpl(
    QString &lastError,
    const QString &inputFile,
    const QString &outputFile,
    const QVector<QImage> &flattenedPageImages,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    QPDF pdf;
    pdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

    QPDFPageDocumentHelper pageHelper(pdf);
    const auto pages = pageHelper.getAllPages();
    if (pages.empty()) {
        lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
        return false;
    }

    if (pageSizesPoints.size() != static_cast<int>(pages.size()) || flattenedPageImages.size() != pageSizesPoints.size()) {
        lastError = QStringLiteral("Seitendaten stimmen nicht mit dem Dokument überein.");
        return false;
    }

    for (int pageIndex = 0; pageIndex < flattenedPageImages.size(); ++pageIndex) {
        const QImage &pageImage = flattenedPageImages.at(pageIndex);
        if (pageImage.isNull()) {
            continue;
        }

        QImage rgbImage = pageImage.convertToFormat(QImage::Format_RGB888);
        if (rgbImage.isNull()) {
            lastError = QStringLiteral("Redigierte Seitenabbildung konnte nicht umgewandelt werden.");
            return false;
        }

        QByteArray imageBytes;
        imageBytes.reserve(rgbImage.width() * rgbImage.height() * 3);
        for (int y = 0; y < rgbImage.height(); ++y) {
            imageBytes.append(reinterpret_cast<const char *>(rgbImage.constScanLine(y)), rgbImage.width() * 3);
        }

        QPDFObjectHandle image = pdf.newStream(imageBytes.toStdString());
        QPDFObjectHandle imageDict = image.getDict();
        imageDict.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
        imageDict.replaceKey("/Subtype", QPDFObjectHandle::newName("/Image"));
        imageDict.replaceKey("/Width", QPDFObjectHandle::newInteger(rgbImage.width()));
        imageDict.replaceKey("/Height", QPDFObjectHandle::newInteger(rgbImage.height()));
        imageDict.replaceKey("/ColorSpace", QPDFObjectHandle::newName("/DeviceRGB"));
        imageDict.replaceKey("/BitsPerComponent", QPDFObjectHandle::newInteger(8));

        const QSizeF pageSize = pageSizesPoints.at(pageIndex);
        const std::string content =
            "q\n"
            + std::to_string(pageSize.width()) + " 0 0 " + std::to_string(pageSize.height()) + " 0 0 cm\n"
            + "/Im1 Do\n"
            + "Q\n";

        QPDFObjectHandle contents = pdf.newStream(content);
        QPDFObjectHandle resources = QPDFObjectHandle::newDictionary();
        resources.replaceKey("/ProcSet", QPDFObjectHandle::parse("[/PDF /ImageC]"));
        QPDFObjectHandle xObject = QPDFObjectHandle::newDictionary();
        xObject.replaceKey("/Im1", image);
        resources.replaceKey("/XObject", xObject);

        QPDFPageObjectHelper page = pages.at(static_cast<std::size_t>(pageIndex));
        QPDFObjectHandle pageObject = page.getObjectHandle();
        pageObject.replaceKey("/Contents", contents);
        pageObject.replaceKey("/Resources", resources);
        pageObject.removeKey("/Annots");
        pageObject.removeKey("/Rotate");
    }

    QPDFWriter writer(pdf, outputFile.toLocal8Bit().constData());
    writer.write();
    return true;
}

bool writePageOrder(
    QString &lastError,
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pageOrder,
    const QByteArray &password)
{
    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        if (pageOrder.size() != static_cast<int>(pages.size())) {
            lastError = QStringLiteral("Die neue Seitenreihenfolge ist unvollständig.");
            return false;
        }

        QVector<int> sortedOrder = pageOrder;
        std::sort(sortedOrder.begin(), sortedOrder.end());
        for (int index = 0; index < sortedOrder.size(); ++index) {
            if (sortedOrder.at(index) != index) {
                lastError = QStringLiteral("Die neue Seitenreihenfolge ist ungültig.");
                return false;
            }
        }

        QPDF outputPdf;
        outputPdf.emptyPDF();
        QPDFPageDocumentHelper outputPages(outputPdf);
        for (int pageIndex : pageOrder) {
            outputPages.addPage(pages.at(static_cast<std::size_t>(pageIndex)), false);
        }

        QPDFWriter writer(outputPdf, outputFile.toLocal8Bit().constData());
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
}
#endif
}

bool QPdfOperations::mergeFiles(const QStringList &inputFiles, const QString &outputFile)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFiles.size() < 2) {
        m_lastError = QStringLiteral("Zum Zusammenführen werden mindestens zwei PDF-Dateien benötigt.");
        return false;
    }

    if (outputFile.isEmpty()) {
        m_lastError = QStringLiteral("Kein Ausgabepfad angegeben.");
        return false;
    }

    try {
        QPDF outputPdf;
        outputPdf.emptyPDF();
        QPDFPageDocumentHelper outputPages(outputPdf);

        for (const QString &inputFile : inputFiles) {
            QPDF inputPdf;
            inputPdf.processFile(inputFile.toLocal8Bit().constData());

            QPDFPageDocumentHelper inputPages(inputPdf);
            for (auto &page : inputPages.getAllPages()) {
                outputPages.addPage(page, false);
            }
        }

        QPDFWriter writer(outputPdf, outputFile.toLocal8Bit().constData());
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFiles)
    Q_UNUSED(outputFile)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::splitDocument(
    const QString &inputFile,
    const QString &outputDirectory,
    const QString &baseName,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFile.isEmpty() || outputDirectory.isEmpty()) {
        m_lastError = QStringLiteral("Eingabedatei oder Ausgabeverzeichnis fehlt.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            m_lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        const QFileInfo inputInfo(inputFile);
        const QString safeBaseName = baseName.isEmpty() ? inputInfo.completeBaseName() : baseName;
        QDir outputDir(outputDirectory);
        if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
            m_lastError = QStringLiteral("Ausgabeverzeichnis konnte nicht erstellt werden.");
            return false;
        }

        for (std::size_t index = 0; index < pages.size(); ++index) {
            QPDF outputPdf;
            outputPdf.emptyPDF();
            QPDFPageDocumentHelper outputPages(outputPdf);
            outputPages.addPage(pages.at(index), false);

            const QString outputPath = outputDir.filePath(
                QStringLiteral("%1_page_%2.pdf").arg(safeBaseName).arg(static_cast<int>(index + 1), 3, 10, QLatin1Char('0'))
            );

            QPDFWriter writer(outputPdf, outputPath.toLocal8Bit().constData());
            writer.write();
        }

        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputDirectory)
    Q_UNUSED(baseName)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::encryptFile(
    const QString &inputFile,
    const QString &outputFile,
    const QByteArray &userPassword,
    const QByteArray &ownerPassword,
    const QByteArray &inputPassword)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        m_lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    if (ownerPassword.isEmpty()) {
        m_lastError = QStringLiteral("Owner-Passwort darf nicht leer sein.");
        return false;
    }

    try {
        QPDF pdf;
        pdf.processFile(inputFile.toLocal8Bit().constData(), inputPassword.isEmpty() ? nullptr : inputPassword.constData());

        QPDFWriter writer(pdf, outputFile.toLocal8Bit().constData());
        writer.setR6EncryptionParameters(
            userPassword.constData(),
            ownerPassword.constData(),
            true,
            true,
            true,
            true,
            true,
            true,
            qpdf_r3p_full,
            true);
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(userPassword)
    Q_UNUSED(ownerPassword)
    Q_UNUSED(inputPassword)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::reorderPages(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pageOrder,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();
    return writePageOrder(m_lastError, inputFile, outputFile, pageOrder, password);
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(pageOrder)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::deletePages(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pagesToDelete,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (pagesToDelete.isEmpty()) {
        m_lastError = QStringLiteral("Es wurden keine Seiten zum Löschen angegeben.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());
        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            m_lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        QVector<int> deleteOrder = pagesToDelete;
        std::sort(deleteOrder.begin(), deleteOrder.end());
        deleteOrder.erase(std::unique(deleteOrder.begin(), deleteOrder.end()), deleteOrder.end());

        QVector<int> remainingPages;
        remainingPages.reserve(static_cast<int>(pages.size()));
        for (int index = 0; index < static_cast<int>(pages.size()); ++index) {
            if (!deleteOrder.contains(index)) {
                remainingPages.append(index);
            }
        }

        if (remainingPages.isEmpty()) {
            m_lastError = QStringLiteral("Mindestens eine Seite muss im Dokument verbleiben.");
            return false;
        }

        return writePageOrder(m_lastError, inputFile, outputFile, remainingPages, password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(pagesToDelete)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::saveAnnotations(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<PdfAnnotation> &annotations,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    try {
        return saveAnnotationsAndFormsImpl(
            m_lastError,
            inputFile,
            outputFile,
            annotations,
            {},
            {},
            pageSizesPoints,
            password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(annotations)
    Q_UNUSED(pageSizesPoints)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::saveAnnotationsAndForms(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<PdfAnnotation> &annotations,
    const QVector<PdfFormField> &formFields,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    try {
        return saveAnnotationsAndFormsImpl(
            m_lastError,
            inputFile,
            outputFile,
            annotations,
            formFields,
            {},
            pageSizesPoints,
            password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(annotations)
    Q_UNUSED(formFields)
    Q_UNUSED(pageSizesPoints)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::saveEditedState(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<PdfAnnotation> &annotations,
    const QVector<PdfFormField> &formFields,
    const QVector<PdfRedaction> &redactions,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    try {
        return saveAnnotationsAndFormsImpl(
            m_lastError,
            inputFile,
            outputFile,
            annotations,
            formFields,
            redactions,
            pageSizesPoints,
            password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(annotations)
    Q_UNUSED(formFields)
    Q_UNUSED(redactions)
    Q_UNUSED(pageSizesPoints)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::saveDestructiveRedactedState(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<QImage> &flattenedPageImages,
    const QVector<QSizeF> &pageSizesPoints,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    try {
        return saveDestructiveRedactedStateImpl(
            m_lastError,
            inputFile,
            outputFile,
            flattenedPageImages,
            pageSizesPoints,
            password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(flattenedPageImages)
    Q_UNUSED(pageSizesPoints)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}
