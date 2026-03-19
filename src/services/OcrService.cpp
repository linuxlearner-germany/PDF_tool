#include "services/OcrService.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

bool OcrService::isAvailable()
{
    return !QStandardPaths::findExecutable(QStringLiteral("tesseract")).isEmpty();
}

QString OcrService::recognizeImage(
    const QImage &image,
    QString *errorMessage,
    const QString &language,
    int pageSegmentationMode)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if (image.isNull()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Kein Bild für OCR verfügbar.");
        }
        return {};
    }

    const QString executable = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    if (executable.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Tesseract wurde nicht gefunden.");
        }
        return {};
    }

    QTemporaryDir temporaryDir;
    if (!temporaryDir.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Temporäres OCR-Verzeichnis konnte nicht angelegt werden.");
        }
        return {};
    }

    const QString inputPath = temporaryDir.filePath(QStringLiteral("ocr_input.png"));
    const QString outputBasePath = temporaryDir.filePath(QStringLiteral("ocr_output"));
    const QString outputTextPath = outputBasePath + QStringLiteral(".txt");

    if (!image.save(inputPath, "PNG")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Eingabebild konnte nicht geschrieben werden.");
        }
        return {};
    }

    QProcess process;
    process.setProgram(executable);
    process.setArguments({
        inputPath,
        outputBasePath,
        QStringLiteral("-l"),
        language,
        QStringLiteral("--psm"),
        QString::number(pageSegmentationMode),
        QStringLiteral("quiet")
    });
    process.start();

    if (!process.waitForStarted()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Tesseract konnte nicht gestartet werden.");
        }
        return {};
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Zeitlimit überschritten.");
        }
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage) {
            const QString stderrText = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
            *errorMessage = stderrText.isEmpty()
                ? QStringLiteral("Tesseract konnte den Text nicht erkennen.")
                : stderrText;
        }
        return {};
    }

    QFile outputFile(outputTextPath);
    if (!outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Ergebnisdatei konnte nicht gelesen werden.");
        }
        return {};
    }

    const QString text = QString::fromUtf8(outputFile.readAll()).trimmed();
    if (text.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("OCR hat keinen Text erkannt.");
    }
    return text;
}
