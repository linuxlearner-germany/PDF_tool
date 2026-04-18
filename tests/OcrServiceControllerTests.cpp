#include <QtTest>

#include <QSignalSpy>
#include <QThread>

#include "document/OcrServiceController.h"

class OcrServiceControllerTests : public QObject
{
    Q_OBJECT

private slots:
    void staleResultsAreIgnored();
    void invalidatedSessionSuppressesPendingResult();
};

void OcrServiceControllerTests::staleResultsAreIgnored()
{
    OcrServiceController controller(
        [](const QImage &, int psm) {
            OcrJobResult result;
            if (psm == 3) {
                QThread::msleep(60);
                result.text = QStringLiteral("stale");
            } else {
                QThread::msleep(10);
                result.text = QStringLiteral("fresh");
            }
            return result;
        },
        []() {
            CapabilityState state;
            state.available = true;
            return state;
        });

    QSignalSpy completedSpy(&controller, &OcrServiceController::requestCompleted);
    QSignalSpy failedSpy(&controller, &OcrServiceController::requestFailed);

    QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    const int firstId = controller.startRequest({image, QStringLiteral("busy"), QStringLiteral("first"), 3});
    const int secondId = controller.startRequest({image, QStringLiteral("busy"), QStringLiteral("second"), 6});

    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);

    const QList<QVariant> arguments = completedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), secondId);
    QCOMPARE(arguments.at(1).toString(), QStringLiteral("second"));
    QCOMPARE(arguments.at(2).toString(), QStringLiteral("fresh"));
    QVERIFY(firstId != secondId);
    QCOMPARE(controller.activeRequestId(), secondId);
}

void OcrServiceControllerTests::invalidatedSessionSuppressesPendingResult()
{
    OcrServiceController controller(
        [](const QImage &, int) {
            QThread::msleep(40);
            OcrJobResult result;
            result.text = QStringLiteral("late");
            return result;
        },
        []() {
            CapabilityState state;
            state.available = true;
            return state;
        });

    QSignalSpy completedSpy(&controller, &OcrServiceController::requestCompleted);
    QSignalSpy failedSpy(&controller, &OcrServiceController::requestFailed);

    QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    controller.startRequest({image, QStringLiteral("busy"), QStringLiteral("first"), 3});
    controller.invalidateActiveSession();

    QTest::qWait(120);

    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
    QVERIFY(controller.activeRequestId() > 0);
}

QTEST_MAIN(OcrServiceControllerTests)

#include "OcrServiceControllerTests.moc"
