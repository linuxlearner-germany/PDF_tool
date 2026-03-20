#pragma once

#include <QJsonArray>
#include <QPointF>
#include <QString>
#include <QVector>

#include "document/PdfDocumentTypes.h"

class RedactionModel
{
public:
    void clear();
    bool add(int pageIndex, const QRectF &pageRect, const QString &reason = QString());
    bool remove(const QString &redactionId);
    void clearSelection();
    bool selectAt(int pageIndex, const QPointF &pagePoint);
    QString selectedRedactionId() const;
    bool hasSelectedRedaction() const;
    bool remapPages(const QVector<int> &newOrder);

    QVector<PdfRedaction> redactions() const;
    QVector<PdfRedaction> redactionsForPage(int pageIndex) const;
    bool hasRedactions() const;
    QJsonArray toJson() const;
    bool fromJson(const QJsonArray &redactionsArray);

private:
    QVector<PdfRedaction> m_redactions;
};
