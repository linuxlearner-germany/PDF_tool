#pragma once

#include <functional>

#include <QFutureWatcher>
#include <QImage>
#include <QObject>

#include "document/PdfDocumentTypes.h"

struct OcrJobResult
{
    QString text;
    QString errorMessage;
};

struct OcrRequest
{
    QImage image;
    QString busyMessage;
    QString resultTitle;
    int pageSegmentationMode {6};
};

class OcrServiceController : public QObject
{
    Q_OBJECT

public:
    using CapabilityProvider = std::function<CapabilityState()>;
    using RecognizeFunction = std::function<OcrJobResult(const QImage &, int)>;

    explicit OcrServiceController(
        RecognizeFunction recognizeFunction = {},
        CapabilityProvider capabilityProvider = {},
        QObject *parent = nullptr);

    CapabilityState capabilityState() const;
    bool isBusy() const;
    int activeRequestId() const;
    void invalidateActiveSession();
    int startRequest(const OcrRequest &request);

signals:
    void busyStateChanged(bool busy, const QString &message);
    void requestCompleted(int requestId, const QString &title, const QString &text);
    void requestFailed(int requestId, const QString &errorMessage);

private:
    CapabilityProvider m_capabilityProvider;
    RecognizeFunction m_recognizeFunction;
    int m_nextRequestId {1};
    int m_activeRequestId {0};
    int m_pendingRequests {0};
};
