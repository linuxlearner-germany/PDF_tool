#pragma once

#include <QString>
#include <QStringList>

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

private:
    QString m_lastError;
};
