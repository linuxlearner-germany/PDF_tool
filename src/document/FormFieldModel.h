#pragma once

#include <QJsonArray>
#include <QPointF>
#include <QString>
#include <QVector>

#include "document/PdfDocumentTypes.h"

class FormFieldModel
{
public:
    void clear();
    void setFields(const QVector<PdfFormField> &fields);

    QVector<PdfFormField> fields() const;
    QVector<PdfFormField> fieldsForPage(int pageIndex) const;
    bool setTextValue(const QString &fieldId, const QString &text);
    bool setChecked(const QString &fieldId, bool checked);
    QString fieldIdAt(int pageIndex, const QPointF &pagePoint) const;
    bool remapPages(const QVector<int> &newOrder);

    QJsonArray toJson() const;
    bool fromJson(const QJsonArray &valuesArray);

private:
    PdfFormField *findMutable(const QString &fieldId);

    QVector<PdfFormField> m_fields;
};
