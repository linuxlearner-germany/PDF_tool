#include "document/FormFieldModel.h"

#include <QRegularExpression>

#include <QJsonObject>

void FormFieldModel::clear()
{
    m_fields.clear();
}

void FormFieldModel::setFields(const QVector<PdfFormField> &fields)
{
    m_fields = fields;
}

QVector<PdfFormField> FormFieldModel::fields() const
{
    return m_fields;
}

QVector<PdfFormField> FormFieldModel::fieldsForPage(int pageIndex) const
{
    QVector<PdfFormField> fields;
    for (const PdfFormField &field : m_fields) {
        if (field.pageIndex == pageIndex) {
            fields.append(field);
        }
    }
    return fields;
}

bool FormFieldModel::setTextValue(const QString &fieldId, const QString &text)
{
    if (PdfFormField *field = findMutable(fieldId)) {
        if (field->kind == PdfFormFieldKind::Text && !field->readOnly) {
            field->textValue = text;
            return true;
        }
    }
    return false;
}

bool FormFieldModel::setChecked(const QString &fieldId, bool checked)
{
    if (PdfFormField *field = findMutable(fieldId)) {
        if (field->kind == PdfFormFieldKind::CheckBox && !field->readOnly) {
            field->checked = checked;
            return true;
        }
    }
    return false;
}

QString FormFieldModel::fieldIdAt(int pageIndex, const QPointF &pagePoint) const
{
    for (qsizetype index = m_fields.size() - 1; index >= 0; --index) {
        const PdfFormField &field = m_fields.at(index);
        if (field.pageIndex == pageIndex && field.pageRect.contains(pagePoint)) {
            return field.id;
        }
    }
    return {};
}

bool FormFieldModel::remapPages(const QVector<int> &newOrder)
{
    static const QRegularExpression pageIdPattern(QStringLiteral("^page_(\\d+)_field_(.+)$"));

    QVector<PdfFormField> remappedFields;
    remappedFields.reserve(m_fields.size());

    for (const PdfFormField &field : std::as_const(m_fields)) {
        const int newPageIndex = newOrder.indexOf(field.pageIndex);
        if (newPageIndex < 0) {
            continue;
        }

        PdfFormField remapped = field;
        remapped.pageIndex = newPageIndex;

        const QRegularExpressionMatch match = pageIdPattern.match(field.id);
        if (match.hasMatch()) {
            remapped.id = QStringLiteral("page_%1_field_%2")
                              .arg(newPageIndex)
                              .arg(match.captured(2));
        }

        remappedFields.append(remapped);
    }

    m_fields = remappedFields;
    return true;
}

QJsonArray FormFieldModel::toJson() const
{
    QJsonArray valuesArray;

    for (const PdfFormField &field : m_fields) {
        QJsonObject fieldObject;
        fieldObject.insert(QStringLiteral("id"), field.id);
        fieldObject.insert(QStringLiteral("kind"), static_cast<int>(field.kind));
        fieldObject.insert(QStringLiteral("textValue"), field.textValue);
        fieldObject.insert(QStringLiteral("checked"), field.checked);
        valuesArray.append(fieldObject);
    }

    return valuesArray;
}

bool FormFieldModel::fromJson(const QJsonArray &valuesArray)
{
    for (const QJsonValue &value : valuesArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject fieldObject = value.toObject();
        const QString fieldId = fieldObject.value(QStringLiteral("id")).toString();
        if (PdfFormField *field = findMutable(fieldId)) {
            field->textValue = fieldObject.value(QStringLiteral("textValue")).toString(field->textValue);
            field->checked = fieldObject.value(QStringLiteral("checked")).toBool(field->checked);
        }
    }

    return true;
}

PdfFormField *FormFieldModel::findMutable(const QString &fieldId)
{
    for (PdfFormField &field : m_fields) {
        if (field.id == fieldId) {
            return &field;
        }
    }
    return nullptr;
}
