#pragma once

#include <QImage>
#include <QString>

class OcrService
{
public:
    static bool isAvailable();
    static QString recognizeImage(
        const QImage &image,
        QString *errorMessage,
        const QString &language = QStringLiteral("deu+eng"),
        int pageSegmentationMode = 6);
};
