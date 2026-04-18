#include "document/AtomicFileTransaction.h"

#include <QFile>

bool AtomicFileTransaction::replaceFile(
    const QString &stagedPath,
    const QString &targetPath,
    const QString &backupSuffix,
    QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    const QString backupPath = targetPath + backupSuffix;
    QFile::remove(backupPath);

    if (QFile::exists(targetPath) && !QFile::rename(targetPath, backupPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Originaldatei konnte nicht gesichert werden.");
        }
        return false;
    }

    if (!QFile::rename(stagedPath, targetPath)) {
        if (QFile::exists(backupPath)) {
            QFile::rename(backupPath, targetPath);
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("Temporaere Datei konnte nicht atomar uebernommen werden.");
        }
        return false;
    }

    QFile::remove(backupPath);
    return true;
}

bool AtomicFileTransaction::removeFileIfExists(const QString &path, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if (!QFile::exists(path)) {
        return true;
    }

    if (!QFile::remove(path)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Datei konnte nicht entfernt werden: %1").arg(path);
        }
        return false;
    }

    return true;
}
