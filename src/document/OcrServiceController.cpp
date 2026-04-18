#include "document/OcrServiceController.h"

#include <QImage>

#include <QtConcurrent/QtConcurrent>
#include <QtGlobal>

#include "services/OcrService.h"

namespace
{
OcrJobResult defaultRecognizeImage(const QImage &image, int pageSegmentationMode)
{
    OcrJobResult result;
    result.text = OcrService::recognizeImage(image, &result.errorMessage, QStringLiteral("deu+eng"), pageSegmentationMode);
    return result;
}
}

OcrServiceController::OcrServiceController(
    RecognizeFunction recognizeFunction,
    CapabilityProvider capabilityProvider,
    QObject *parent)
    : QObject(parent)
    , m_capabilityProvider(capabilityProvider ? std::move(capabilityProvider) : CapabilityProvider(OcrService::capabilityState))
    , m_recognizeFunction(recognizeFunction ? std::move(recognizeFunction) : RecognizeFunction(defaultRecognizeImage))
{
}

CapabilityState OcrServiceController::capabilityState() const
{
    return m_capabilityProvider();
}

bool OcrServiceController::isBusy() const
{
    return m_pendingRequests > 0;
}

int OcrServiceController::activeRequestId() const
{
    return m_activeRequestId;
}

int OcrServiceController::startRequest(const OcrRequest &request)
{
    const CapabilityState state = capabilityState();
    const int requestId = m_nextRequestId++;
    if (!state.available) {
        emit requestFailed(requestId, state.error);
        return requestId;
    }

    m_activeRequestId = requestId;
    ++m_pendingRequests;
    emit busyStateChanged(true, request.busyMessage);

    auto *watcher = new QFutureWatcher<OcrJobResult>(this);
    connect(watcher, &QFutureWatcher<OcrJobResult>::finished, this, [this, watcher, requestId, title = request.resultTitle]() {
        const OcrJobResult result = watcher->result();
        watcher->deleteLater();
        m_pendingRequests = qMax(0, m_pendingRequests - 1);
        if (m_pendingRequests == 0) {
            emit busyStateChanged(false, QString());
        }

        if (requestId != m_activeRequestId) {
            return;
        }

        if (result.text.isEmpty()) {
            emit requestFailed(requestId, result.errorMessage.isEmpty()
                ? QStringLiteral("OCR hat keinen Text erkannt.")
                : result.errorMessage);
            return;
        }

        emit requestCompleted(requestId, title, result.text);
    });

    watcher->setFuture(QtConcurrent::run([recognize = m_recognizeFunction, image = request.image, psm = request.pageSegmentationMode]() {
        return recognize(image, psm);
    }));
    return requestId;
}
