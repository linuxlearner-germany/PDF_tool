#pragma once

#include <QString>

class AtomicFileTransaction
{
public:
    static bool replaceFile(
        const QString &stagedPath,
        const QString &targetPath,
        const QString &backupSuffix,
        QString *errorMessage = nullptr);
    static bool removeFileIfExists(const QString &path, QString *errorMessage = nullptr);
};
