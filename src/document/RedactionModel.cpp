#include "document/RedactionModel.h"

#include <QUuid>

#include <QJsonObject>

namespace
{
QString generateRedactionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
}

void RedactionModel::clear()
{
    m_redactions.clear();
}

bool RedactionModel::add(int pageIndex, const QRectF &pageRect, const QString &reason)
{
    if (pageIndex < 0 || pageRect.isEmpty()) {
        return false;
    }

    PdfRedaction redaction;
    redaction.id = generateRedactionId();
    redaction.pageIndex = pageIndex;
    redaction.pageRect = pageRect.normalized();
    redaction.reason = reason.trimmed();

    clearSelection();
    redaction.selected = true;
    m_redactions.append(redaction);
    return true;
}

bool RedactionModel::remove(const QString &redactionId)
{
    for (qsizetype index = 0; index < m_redactions.size(); ++index) {
        if (m_redactions.at(index).id == redactionId) {
            m_redactions.removeAt(index);
            return true;
        }
    }
    return false;
}

void RedactionModel::clearSelection()
{
    for (PdfRedaction &redaction : m_redactions) {
        redaction.selected = false;
    }
}

bool RedactionModel::selectAt(int pageIndex, const QPointF &pagePoint)
{
    clearSelection();

    for (qsizetype index = m_redactions.size() - 1; index >= 0; --index) {
        PdfRedaction &redaction = m_redactions[index];
        if (redaction.pageIndex == pageIndex && redaction.pageRect.contains(pagePoint)) {
            redaction.selected = true;
            return true;
        }
    }
    return false;
}

QString RedactionModel::selectedRedactionId() const
{
    for (const PdfRedaction &redaction : m_redactions) {
        if (redaction.selected) {
            return redaction.id;
        }
    }
    return {};
}

bool RedactionModel::hasSelectedRedaction() const
{
    return !selectedRedactionId().isEmpty();
}

QVector<PdfRedaction> RedactionModel::redactionsForPage(int pageIndex) const
{
    QVector<PdfRedaction> redactions;
    for (const PdfRedaction &redaction : m_redactions) {
        if (redaction.pageIndex == pageIndex) {
            redactions.append(redaction);
        }
    }
    return redactions;
}

bool RedactionModel::hasRedactions() const
{
    return !m_redactions.isEmpty();
}

QJsonArray RedactionModel::toJson() const
{
    QJsonArray redactionsArray;

    for (const PdfRedaction &redaction : m_redactions) {
        QJsonObject redactionObject;
        redactionObject.insert(QStringLiteral("id"), redaction.id);
        redactionObject.insert(QStringLiteral("pageIndex"), redaction.pageIndex);
        redactionObject.insert(QStringLiteral("reason"), redaction.reason);
        redactionObject.insert(QStringLiteral("x"), redaction.pageRect.x());
        redactionObject.insert(QStringLiteral("y"), redaction.pageRect.y());
        redactionObject.insert(QStringLiteral("width"), redaction.pageRect.width());
        redactionObject.insert(QStringLiteral("height"), redaction.pageRect.height());
        redactionsArray.append(redactionObject);
    }

    return redactionsArray;
}

bool RedactionModel::fromJson(const QJsonArray &redactionsArray)
{
    clear();

    for (const QJsonValue &value : redactionsArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject redactionObject = value.toObject();
        PdfRedaction redaction;
        redaction.id = redactionObject.value(QStringLiteral("id")).toString(generateRedactionId());
        redaction.pageIndex = redactionObject.value(QStringLiteral("pageIndex")).toInt(-1);
        redaction.reason = redactionObject.value(QStringLiteral("reason")).toString();
        redaction.pageRect = QRectF(redactionObject.value(QStringLiteral("x")).toDouble(),
                                    redactionObject.value(QStringLiteral("y")).toDouble(),
                                    redactionObject.value(QStringLiteral("width")).toDouble(),
                                    redactionObject.value(QStringLiteral("height")).toDouble());

        if (redaction.pageIndex >= 0 && !redaction.pageRect.isEmpty()) {
            m_redactions.append(redaction);
        }
    }

    return true;
}
