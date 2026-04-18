#pragma once

#include <QImage>
#include <QRectF>
#include <QString>
#include <QVector>

struct OcrWordBox
{
    QRectF imageRect;
    QString text;
    int confidence {-1};
};

class OcrService
{
public:
    static bool isAvailable();
    static QString availabilityError();
    static CapabilityState capabilityState();
    static QString recognizeImage(
        const QImage &image,
        QString *errorMessage,
        const QString &language = QStringLiteral("deu+eng"),
        int pageSegmentationMode = 6);
    static QVector<OcrWordBox> recognizeImageWords(
        const QImage &image,
        QString *errorMessage,
        const QString &language = QStringLiteral("deu+eng"),
        int pageSegmentationMode = 6);
};
