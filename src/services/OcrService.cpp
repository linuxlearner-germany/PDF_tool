#include "services/OcrService.h"

#include <QStringList>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace
{
bool prepareOcrInput(const QImage &image, QString *errorMessage, QTemporaryDir &temporaryDir, QString &inputPath, QString &outputBasePath)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    if (image.isNull()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Kein Bild für OCR verfügbar.");
        }
        return false;
    }

    if (!temporaryDir.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Temporäres OCR-Verzeichnis konnte nicht angelegt werden.");
        }
        return false;
    }

    inputPath = temporaryDir.filePath(QStringLiteral("ocr_input.png"));
    outputBasePath = temporaryDir.filePath(QStringLiteral("ocr_output"));
    if (!image.save(inputPath, "PNG")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Eingabebild konnte nicht geschrieben werden.");
        }
        return false;
    }

    return true;
}

bool runTesseract(
    const QString &executable,
    const QString &inputPath,
    const QString &outputBasePath,
    const QStringList &extraArguments,
    QString *errorMessage)
{
    QProcess process;
    QStringList arguments = {
        inputPath,
        outputBasePath,
    };
    arguments += extraArguments;
    arguments << QStringLiteral("quiet");

    process.setProgram(executable);
    process.setArguments(arguments);
    process.start();

    if (!process.waitForStarted()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Tesseract konnte nicht gestartet werden.");
        }
        return false;
    }

    if (!process.waitForFinished(30000)) {
        process.kill();
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Zeitlimit überschritten.");
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage) {
            const QString stderrText = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
            *errorMessage = stderrText.isEmpty()
                ? QStringLiteral("Tesseract konnte den Text nicht erkennen.")
                : stderrText;
        }
        return false;
    }

    return true;
}
}

bool OcrService::isAvailable()
{
    return !QStandardPaths::findExecutable(QStringLiteral("tesseract")).isEmpty();
}

QString OcrService::availabilityError()
{
    return isAvailable()
        ? QString()
        : QStringLiteral("Tesseract wurde nicht gefunden. OCR ist deaktiviert, bis das Binary im PATH verfuegbar ist.");
}

QString OcrService::recognizeImage(
    const QImage &image,
    QString *errorMessage,
    const QString &language,
    int pageSegmentationMode)
{
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    if (executable.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Tesseract wurde nicht gefunden.");
        }
        return {};
    }

    QTemporaryDir temporaryDir;
    QString inputPath;
    QString outputBasePath;
    if (!prepareOcrInput(image, errorMessage, temporaryDir, inputPath, outputBasePath)) {
        return {};
    }

    const QString outputTextPath = outputBasePath + QStringLiteral(".txt");
    if (!runTesseract(
            executable,
            inputPath,
            outputBasePath,
            {
                QStringLiteral("-l"),
                language,
                QStringLiteral("--psm"),
                QString::number(pageSegmentationMode),
            },
            errorMessage)) {
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

QVector<OcrWordBox> OcrService::recognizeImageWords(
    const QImage &image,
    QString *errorMessage,
    const QString &language,
    int pageSegmentationMode)
{
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("tesseract"));
    if (executable.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Tesseract wurde nicht gefunden.");
        }
        return {};
    }

    QTemporaryDir temporaryDir;
    QString inputPath;
    QString outputBasePath;
    if (!prepareOcrInput(image, errorMessage, temporaryDir, inputPath, outputBasePath)) {
        return {};
    }

    const QString outputTsvPath = outputBasePath + QStringLiteral(".tsv");
    if (!runTesseract(
            executable,
            inputPath,
            outputBasePath,
            {
                QStringLiteral("-l"),
                language,
                QStringLiteral("--psm"),
                QString::number(pageSegmentationMode),
                QStringLiteral("tsv"),
            },
            errorMessage)) {
        return {};
    }

    QFile outputFile(outputTsvPath);
    if (!outputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OCR-Positionsdatei konnte nicht gelesen werden.");
        }
        return {};
    }

    QVector<OcrWordBox> words;
    const QStringList lines = QString::fromUtf8(outputFile.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex) {
        const QString line = lines.at(lineIndex).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList columns = line.split(QLatin1Char('\t'));
        if (columns.size() < 12) {
            continue;
        }

        bool levelOk = false;
        const int level = columns.at(0).toInt(&levelOk);
        if (!levelOk || level != 5) {
            continue;
        }

        bool leftOk = false;
        bool topOk = false;
        bool widthOk = false;
        bool heightOk = false;
        bool confidenceOk = false;
        const int left = columns.at(6).toInt(&leftOk);
        const int top = columns.at(7).toInt(&topOk);
        const int width = columns.at(8).toInt(&widthOk);
        const int height = columns.at(9).toInt(&heightOk);
        const int confidence = columns.at(10).toInt(&confidenceOk);
        const QString text = columns.mid(11).join(QStringLiteral("\t")).trimmed();

        if (!leftOk || !topOk || !widthOk || !heightOk || width <= 0 || height <= 0 || text.isEmpty()) {
            continue;
        }

        OcrWordBox word;
        word.imageRect = QRectF(left, top, width, height);
        word.text = text;
        word.confidence = confidenceOk ? confidence : -1;
        words.append(word);
    }

    if (words.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("OCR hat keine Wortpositionen erkannt.");
    }

    return words;
}
