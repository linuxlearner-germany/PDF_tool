#include "ui/MainWindow.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>
#include <QStatusBar>
#include <QTemporaryFile>

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include "document/PdfDocumentController.h"
#include "operations/QPdfOperations.h"
#include "ui/MainWindowSupport.h"

using namespace MainWindowSupport;

void MainWindow::openPdf()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("PDF öffnen"),
        QString(),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    if (!m_documentController->openDocument(filePath)) {
        if (!m_documentController->requiresPassword()) {
            return;
        }

        while (true) {
            const PasswordDialogResult passwordResult = promptForPasswords(
                this,
                QStringLiteral("PDF-Passwort"),
                QStringLiteral("Dieses PDF ist passwortgeschützt. Bitte Passwort eingeben."),
                false,
                true,
                QString(),
                QStringLiteral("Passwort:"));

            if (!passwordResult.accepted) {
                return;
            }

            if (m_documentController->openDocument(filePath, QByteArray(), passwordResult.userPassword.toUtf8())) {
                break;
            }

            showError(m_documentController->lastError());
            if (!m_documentController->requiresPassword()) {
                return;
            }
        }
    }

    refreshNavigationPanels();
    updateWindowTitle();
}

void MainWindow::saveDocument()
{
    m_documentController->saveDocumentState();
}

void MainWindow::mergePdfFiles()
{
    const QStringList inputFiles = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("PDF-Dateien zusammenführen"),
        QString(),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (inputFiles.size() < 2) {
        return;
    }

    const QString outputFile = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Zusammengeführtes PDF speichern"),
        QStringLiteral("merged.pdf"),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (outputFile.isEmpty()) {
        return;
    }

    if (!m_pdfOperations->mergeFiles(inputFiles, outputFile)) {
        showError(m_pdfOperations->lastError());
        return;
    }

    statusBar()->showMessage(QStringLiteral("PDF-Dateien erfolgreich zusammengeführt."), 4000);
}

void MainWindow::splitCurrentPdf()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const QString outputDirectory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Zielordner für geteilte PDF-Dateien wählen")
    );

    if (outputDirectory.isEmpty()) {
        return;
    }

    const QFileInfo inputInfo(m_documentController->documentPath());
    if (!m_pdfOperations->splitDocument(
            m_documentController->documentPath(),
            outputDirectory,
            inputInfo.completeBaseName(),
            m_documentController->currentUserPassword())) {
        showError(m_pdfOperations->lastError());
        return;
    }

    statusBar()->showMessage(QStringLiteral("PDF erfolgreich in Einzelseiten-Dateien geteilt."), 4000);
}

void MainWindow::exportEncryptedPdf()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const PasswordDialogResult passwordResult = promptForPasswords(
        this,
        QStringLiteral("PDF verschlüsseln"),
        QStringLiteral("Bitte User- und Owner-Passwort für die Ausgabedatei angeben."),
        true,
        true,
        QStringLiteral("Owner-Passwort:"),
        QStringLiteral("User-Passwort:"));

    if (!passwordResult.accepted) {
        return;
    }

    if (passwordResult.ownerPassword.isEmpty()) {
        showError(QStringLiteral("Owner-Passwort darf nicht leer sein."));
        return;
    }

    const QFileInfo inputInfo(m_documentController->documentPath());
    const QString outputFile = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Verschlüsseltes PDF speichern"),
        inputInfo.absolutePath() + QLatin1Char('/') + inputInfo.completeBaseName() + QStringLiteral("_encrypted.pdf"),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (outputFile.isEmpty()) {
        return;
    }

    if (!m_pdfOperations->encryptFile(
            m_documentController->documentPath(),
            outputFile,
            passwordResult.userPassword.toUtf8(),
            passwordResult.ownerPassword.toUtf8(),
            m_documentController->currentUserPassword())) {
        showError(m_pdfOperations->lastError());
        return;
    }

    statusBar()->showMessage(QStringLiteral("Verschlüsseltes PDF erfolgreich gespeichert."), 4000);
}

void MainWindow::exportRedactedPdf()
{
    if (!m_documentController->hasDocument() || !m_documentController->hasRedactions()) {
        return;
    }

    const QFileInfo inputInfo(m_documentController->documentPath());
    const QString outputFile = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Geschwärztes PDF exportieren"),
        inputInfo.absolutePath() + QLatin1Char('/') + inputInfo.completeBaseName() + QStringLiteral("_redacted.pdf"),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (outputFile.isEmpty()) {
        return;
    }

    if (!m_documentController->exportRedactedPdf(outputFile)) {
        showError(m_documentController->lastError());
    }
}

void MainWindow::exportEditedPdf()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const QFileInfo inputInfo(m_documentController->documentPath());
    const QString outputFile = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Bearbeitetes PDF exportieren"),
        inputInfo.absolutePath() + QLatin1Char('/') + inputInfo.completeBaseName() + QStringLiteral("_edited.pdf"),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (outputFile.isEmpty()) {
        return;
    }

    if (!m_documentController->exportEditedPdf(outputFile)) {
        showError(m_documentController->lastError());
    }
}

void MainWindow::exportSignedPdf()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    if (!cryptographicSigningAvailable()) {
        showError(QStringLiteral("Kein kryptografisches Signatur-Backend in Poppler verfügbar."));
        return;
    }

    int pageIndex = -1;
    QRectF pageRect;
    QByteArray signatureImageData;
    if (!m_documentController->selectedSignatureAnnotationData(pageIndex, pageRect, signatureImageData)) {
        showError(QStringLiteral("Bitte zuerst eine eingefügte Unterschrift auswählen."));
        return;
    }

    QString certNickname;
    QString certPassword;
    QString reason;
    QString location;
    if (!promptForCryptographicSignature(certNickname, certPassword, reason, location)) {
        return;
    }

    if (certNickname.trimmed().isEmpty()) {
        showError(QStringLiteral("Zertifikat-Nickname darf nicht leer sein."));
        return;
    }

    const QFileInfo inputInfo(m_documentController->documentPath());
    const QString outputFile = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Signiertes PDF speichern"),
        inputInfo.absolutePath() + QLatin1Char('/') + inputInfo.completeBaseName() + QStringLiteral("_signed.pdf"),
        QStringLiteral("PDF Dateien (*.pdf)")
    );

    if (outputFile.isEmpty()) {
        return;
    }

    QTemporaryFile unsignedPdfFile(QDir::tempPath() + QStringLiteral("/pdftool-sign-XXXXXX.pdf"));
    unsignedPdfFile.setAutoRemove(true);
    if (!unsignedPdfFile.open()) {
        showError(QStringLiteral("Temporäre Datei für die Signatur konnte nicht angelegt werden."));
        return;
    }
    const QString unsignedPdfPath = unsignedPdfFile.fileName();
    unsignedPdfFile.close();

    if (!m_documentController->exportEditedPdf(unsignedPdfPath, true)) {
        showError(m_documentController->lastError());
        return;
    }

    if (!signPdfWithPoppler(
            unsignedPdfPath,
            outputFile,
            pageIndex,
            pageRect,
            signatureImageData,
            certNickname.trimmed(),
            certPassword,
            reason.trimmed(),
            location.trimmed())) {
        return;
    }

    statusBar()->showMessage(QStringLiteral("PDF erfolgreich kryptografisch signiert."), 4000);
}

void MainWindow::printDocument()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QFileInfo(m_documentController->documentPath()).fileName());

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(QStringLiteral("PDF drucken"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_documentController->printDocument(&printer);
}

void MainWindow::insertSignature()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Unterschrift auswählen"),
        QString(),
        QStringLiteral("Bilder (*.png *.jpg *.jpeg *.bmp)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    const QImage signatureImage(filePath);
    if (signatureImage.isNull()) {
        showError(QStringLiteral("Unterschriftsbild konnte nicht geladen werden."));
        return;
    }

    if (m_documentController->hasAreaSelection()) {
        m_documentController->addSignatureFromImageToSelection(signatureImage);
    } else {
        m_documentController->addSignatureFromImageAt(m_lastContextImagePoint, signatureImage);
    }
}

void MainWindow::runOcrCurrentPage()
{
    m_documentController->runOcrOnCurrentPage();
}

void MainWindow::runOcrSelection()
{
    m_documentController->runOcrOnSelection();
}
