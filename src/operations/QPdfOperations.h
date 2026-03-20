#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QSizeF>
#include <QVector>

#include "document/PdfDocumentTypes.h"

class QPdfOperations
{
public:
    QPdfOperations() = default;

    bool isAvailable() const;
    QString backendName() const;
    QString lastError() const;

    bool mergeFiles(const QStringList &inputFiles, const QString &outputFile);
    bool splitDocument(
        const QString &inputFile,
        const QString &outputDirectory,
        const QString &baseName = QString(),
        const QByteArray &password = QByteArray());
    bool encryptFile(
        const QString &inputFile,
        const QString &outputFile,
        const QByteArray &userPassword,
        const QByteArray &ownerPassword,
        const QByteArray &inputPassword = QByteArray());
    bool reorderPages(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<int> &pageOrder,
        const QByteArray &password = QByteArray());
    bool deletePages(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<int> &pagesToDelete,
        const QByteArray &password = QByteArray());
    bool saveAnnotations(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<PdfAnnotation> &annotations,
        const QVector<QSizeF> &pageSizesPoints,
        const QByteArray &password = QByteArray());
    bool saveAnnotationsAndForms(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<PdfAnnotation> &annotations,
        const QVector<PdfFormField> &formFields,
        const QVector<QSizeF> &pageSizesPoints,
        const QByteArray &password = QByteArray());
    bool saveEditedState(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<PdfAnnotation> &annotations,
        const QVector<PdfFormField> &formFields,
        const QVector<PdfRedaction> &redactions,
        const QVector<QSizeF> &pageSizesPoints,
        const QByteArray &password = QByteArray());
    bool saveDestructiveRedactedState(
        const QString &inputFile,
        const QString &outputFile,
        const QVector<QImage> &flattenedPageImages,
        const QVector<QSizeF> &pageSizesPoints,
        const QByteArray &password = QByteArray());

private:
    QString m_lastError;
};
