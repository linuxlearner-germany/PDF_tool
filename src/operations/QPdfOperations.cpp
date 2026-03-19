#include "operations/QPdfOperations.h"

#include <algorithm>

#include <QDir>
#include <QFileInfo>

#ifdef PDF_TOOL_HAS_QPDF
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFExc.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFWriter.hh>
#endif

bool QPdfOperations::isAvailable() const
{
#ifdef PDF_TOOL_HAS_QPDF
    return true;
#else
    return false;
#endif
}

QString QPdfOperations::backendName() const
{
    return QStringLiteral("qpdf");
}

QString QPdfOperations::lastError() const
{
    return m_lastError;
}

namespace
{
#ifdef PDF_TOOL_HAS_QPDF
bool writePageOrder(
    QString &lastError,
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pageOrder,
    const QByteArray &password)
{
    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        if (pageOrder.size() != static_cast<int>(pages.size())) {
            lastError = QStringLiteral("Die neue Seitenreihenfolge ist unvollständig.");
            return false;
        }

        QVector<int> sortedOrder = pageOrder;
        std::sort(sortedOrder.begin(), sortedOrder.end());
        for (int index = 0; index < sortedOrder.size(); ++index) {
            if (sortedOrder.at(index) != index) {
                lastError = QStringLiteral("Die neue Seitenreihenfolge ist ungültig.");
                return false;
            }
        }

        QPDF outputPdf;
        outputPdf.emptyPDF();
        QPDFPageDocumentHelper outputPages(outputPdf);
        for (int pageIndex : pageOrder) {
            outputPages.addPage(pages.at(static_cast<std::size_t>(pageIndex)), false);
        }

        QPDFWriter writer(outputPdf, outputFile.toLocal8Bit().constData());
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
}
#endif
}

bool QPdfOperations::mergeFiles(const QStringList &inputFiles, const QString &outputFile)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFiles.size() < 2) {
        m_lastError = QStringLiteral("Zum Zusammenführen werden mindestens zwei PDF-Dateien benötigt.");
        return false;
    }

    if (outputFile.isEmpty()) {
        m_lastError = QStringLiteral("Kein Ausgabepfad angegeben.");
        return false;
    }

    try {
        QPDF outputPdf;
        outputPdf.emptyPDF();
        QPDFPageDocumentHelper outputPages(outputPdf);

        for (const QString &inputFile : inputFiles) {
            QPDF inputPdf;
            inputPdf.processFile(inputFile.toLocal8Bit().constData());

            QPDFPageDocumentHelper inputPages(inputPdf);
            for (auto &page : inputPages.getAllPages()) {
                outputPages.addPage(page, false);
            }
        }

        QPDFWriter writer(outputPdf, outputFile.toLocal8Bit().constData());
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFiles)
    Q_UNUSED(outputFile)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::splitDocument(
    const QString &inputFile,
    const QString &outputDirectory,
    const QString &baseName,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFile.isEmpty() || outputDirectory.isEmpty()) {
        m_lastError = QStringLiteral("Eingabedatei oder Ausgabeverzeichnis fehlt.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());

        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            m_lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        const QFileInfo inputInfo(inputFile);
        const QString safeBaseName = baseName.isEmpty() ? inputInfo.completeBaseName() : baseName;
        QDir outputDir(outputDirectory);
        if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
            m_lastError = QStringLiteral("Ausgabeverzeichnis konnte nicht erstellt werden.");
            return false;
        }

        for (std::size_t index = 0; index < pages.size(); ++index) {
            QPDF outputPdf;
            outputPdf.emptyPDF();
            QPDFPageDocumentHelper outputPages(outputPdf);
            outputPages.addPage(pages.at(index), false);

            const QString outputPath = outputDir.filePath(
                QStringLiteral("%1_page_%2.pdf").arg(safeBaseName).arg(static_cast<int>(index + 1), 3, 10, QLatin1Char('0'))
            );

            QPDFWriter writer(outputPdf, outputPath.toLocal8Bit().constData());
            writer.write();
        }

        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputDirectory)
    Q_UNUSED(baseName)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::encryptFile(
    const QString &inputFile,
    const QString &outputFile,
    const QByteArray &userPassword,
    const QByteArray &ownerPassword,
    const QByteArray &inputPassword)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (inputFile.isEmpty() || outputFile.isEmpty()) {
        m_lastError = QStringLiteral("Eingabe- oder Ausgabedatei fehlt.");
        return false;
    }

    if (ownerPassword.isEmpty()) {
        m_lastError = QStringLiteral("Owner-Passwort darf nicht leer sein.");
        return false;
    }

    try {
        QPDF pdf;
        pdf.processFile(inputFile.toLocal8Bit().constData(), inputPassword.isEmpty() ? nullptr : inputPassword.constData());

        QPDFWriter writer(pdf, outputFile.toLocal8Bit().constData());
        writer.setR6EncryptionParameters(
            userPassword.constData(),
            ownerPassword.constData(),
            true,
            true,
            true,
            true,
            true,
            true,
            qpdf_r3p_full,
            true);
        writer.write();
        return true;
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(userPassword)
    Q_UNUSED(ownerPassword)
    Q_UNUSED(inputPassword)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::reorderPages(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pageOrder,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();
    return writePageOrder(m_lastError, inputFile, outputFile, pageOrder, password);
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(pageOrder)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}

bool QPdfOperations::deletePages(
    const QString &inputFile,
    const QString &outputFile,
    const QVector<int> &pagesToDelete,
    const QByteArray &password)
{
#ifdef PDF_TOOL_HAS_QPDF
    m_lastError.clear();

    if (pagesToDelete.isEmpty()) {
        m_lastError = QStringLiteral("Es wurden keine Seiten zum Löschen angegeben.");
        return false;
    }

    try {
        QPDF inputPdf;
        inputPdf.processFile(inputFile.toLocal8Bit().constData(), password.isEmpty() ? nullptr : password.constData());
        QPDFPageDocumentHelper inputPages(inputPdf);
        const auto pages = inputPages.getAllPages();
        if (pages.empty()) {
            m_lastError = QStringLiteral("Das Dokument enthält keine Seiten.");
            return false;
        }

        QVector<int> deleteOrder = pagesToDelete;
        std::sort(deleteOrder.begin(), deleteOrder.end());
        deleteOrder.erase(std::unique(deleteOrder.begin(), deleteOrder.end()), deleteOrder.end());

        QVector<int> remainingPages;
        remainingPages.reserve(static_cast<int>(pages.size()));
        for (int index = 0; index < static_cast<int>(pages.size()); ++index) {
            if (!deleteOrder.contains(index)) {
                remainingPages.append(index);
            }
        }

        if (remainingPages.isEmpty()) {
            m_lastError = QStringLiteral("Mindestens eine Seite muss im Dokument verbleiben.");
            return false;
        }

        return writePageOrder(m_lastError, inputFile, outputFile, remainingPages, password);
    } catch (const QPDFExc &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    } catch (const std::exception &exception) {
        m_lastError = QString::fromLocal8Bit(exception.what());
        return false;
    }
#else
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(pagesToDelete)
    Q_UNUSED(password)
    m_lastError = QStringLiteral("qpdf ist nicht verfügbar.");
    return false;
#endif
}
