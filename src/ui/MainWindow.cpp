#include "ui/MainWindow.h"

#include <memory>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QModelIndex>
#include <QIcon>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QTextEdit>
#include <QTemporaryFile>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include <poppler-converter.h>
#include <poppler-form.h>
#include <poppler-qt6.h>

#include "document/PdfDocumentController.h"
#include "document/PdfDocumentTypes.h"
#include "operations/QPdfOperations.h"
#include "rendering/PopplerAdapter.h"
#include "ui/PageThumbnailListModel.h"
#include "ui/PdfView.h"

namespace
{
QIcon loadToolbarIcon(const QString &name)
{
    return QIcon(QStringLiteral(":/icons/%1.svg").arg(name));
}

QStringList supportedPdfFontFamilies()
{
    return {
        QStringLiteral("Helvetica"),
        QStringLiteral("Times New Roman"),
        QStringLiteral("Courier"),
    };
}

QString normalizedPdfFontFamily(const QString &fontFamily)
{
    const QString family = fontFamily.trimmed();
    if (family.contains(QStringLiteral("courier"), Qt::CaseInsensitive)) {
        return QStringLiteral("Courier");
    }
    if (family.contains(QStringLiteral("times"), Qt::CaseInsensitive)) {
        return QStringLiteral("Times New Roman");
    }
    return QStringLiteral("Helvetica");
}

struct PasswordDialogResult
{
    QString ownerPassword;
    QString userPassword;
    bool accepted {false};
};

struct TextAnnotationDialogResult
{
    QString text;
    PdfTextStyle style;
    QColor backgroundColor {255, 255, 255, 230};
    bool accepted {false};
};

struct TextStyleDialogResult
{
    PdfTextStyle style;
    bool accepted {false};
};

void updateColorButton(QPushButton *button, const QColor &color)
{
    const QString label = QStringLiteral("%1 (%2)").arg(color.name(QColor::HexRgb), color.alpha());
    button->setText(label);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #666; padding: 4px 8px; }")
                              .arg(color.red())
                              .arg(color.green())
                              .arg(color.blue())
                              .arg(color.alpha())
                              .arg(color.lightnessF() < 0.45 ? QStringLiteral("#ffffff") : QStringLiteral("#000000")));
}

TextAnnotationDialogResult promptForTextAnnotation(
    QWidget *parent,
    const QString &title,
    const QString &label,
    const QString &initialText = QString(),
    const PdfTextStyle &initialStyle = PdfTextStyle(),
    const QColor &initialBackground = QColor(255, 255, 255, 230))
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(520, 420);

    auto *layout = new QVBoxLayout(&dialog);

    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    auto *textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(initialText);
    formLayout->addRow(label, textEdit);

    auto *fontFamilyBox = new QComboBox(&dialog);
    fontFamilyBox->addItems(supportedPdfFontFamilies());
    const QString initialFamily = normalizedPdfFontFamily(initialStyle.fontFamily);
    const int initialFamilyIndex = fontFamilyBox->findText(initialFamily);
    fontFamilyBox->setCurrentIndex(initialFamilyIndex >= 0 ? initialFamilyIndex : 0);
    formLayout->addRow(QStringLiteral("Schriftart:"), fontFamilyBox);

    auto *fontSizeSpin = new QSpinBox(&dialog);
    fontSizeSpin->setRange(6, 96);
    fontSizeSpin->setValue(qRound(initialStyle.fontSize > 0.0 ? initialStyle.fontSize : 12.0));
    formLayout->addRow(QStringLiteral("Schriftgröße:"), fontSizeSpin);

    QColor textColor = initialStyle.textColor.isValid() ? initialStyle.textColor : QColor(Qt::black);
    QColor backgroundColor = initialBackground.isValid() ? initialBackground : QColor(255, 255, 255, 230);

    auto *textColorButton = new QPushButton(&dialog);
    updateColorButton(textColorButton, textColor);
    QObject::connect(textColorButton, &QPushButton::clicked, &dialog, [&dialog, textColorButton, &textColor]() {
        const QColor selected = QColorDialog::getColor(textColor, &dialog, QStringLiteral("Schriftfarbe wählen"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        textColor = selected;
        updateColorButton(textColorButton, textColor);
    });
    formLayout->addRow(QStringLiteral("Schriftfarbe:"), textColorButton);

    auto *backgroundColorButton = new QPushButton(&dialog);
    updateColorButton(backgroundColorButton, backgroundColor);
    QObject::connect(backgroundColorButton, &QPushButton::clicked, &dialog, [&dialog, backgroundColorButton, &backgroundColor]() {
        const QColor selected = QColorDialog::getColor(
            backgroundColor,
            &dialog,
            QStringLiteral("Hintergrundfarbe wählen"),
            QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        backgroundColor = selected;
        updateColorButton(backgroundColorButton, backgroundColor);
    });
    formLayout->addRow(QStringLiteral("Hintergrund:"), backgroundColorButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    TextAnnotationDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (!result.accepted) {
        return result;
    }

    result.text = textEdit->toPlainText().trimmed();
    result.style.textColor = textColor;
    result.style.fontFamily = normalizedPdfFontFamily(fontFamilyBox->currentText());
    result.style.fontSize = fontSizeSpin->value();
    result.backgroundColor = backgroundColor;
    return result;
}

TextStyleDialogResult promptForTextStyle(
    QWidget *parent,
    const QString &title,
    const PdfTextStyle &initialStyle = PdfTextStyle())
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    auto *fontFamilyBox = new QComboBox(&dialog);
    fontFamilyBox->addItems(supportedPdfFontFamilies());
    const QString initialFamily = normalizedPdfFontFamily(initialStyle.fontFamily);
    const int initialFamilyIndex = fontFamilyBox->findText(initialFamily);
    fontFamilyBox->setCurrentIndex(initialFamilyIndex >= 0 ? initialFamilyIndex : 0);
    formLayout->addRow(QStringLiteral("Schriftart:"), fontFamilyBox);

    auto *fontSizeSpin = new QSpinBox(&dialog);
    fontSizeSpin->setRange(6, 96);
    fontSizeSpin->setValue(qRound(initialStyle.fontSize > 0.0 ? initialStyle.fontSize : 12.0));
    formLayout->addRow(QStringLiteral("Schriftgröße:"), fontSizeSpin);

    QColor textColor = initialStyle.textColor.isValid() ? initialStyle.textColor : QColor(Qt::black);
    auto *textColorButton = new QPushButton(&dialog);
    updateColorButton(textColorButton, textColor);
    QObject::connect(textColorButton, &QPushButton::clicked, &dialog, [&dialog, textColorButton, &textColor]() {
        const QColor selected = QColorDialog::getColor(textColor, &dialog, QStringLiteral("Schriftfarbe wählen"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        textColor = selected;
        updateColorButton(textColorButton, textColor);
    });
    formLayout->addRow(QStringLiteral("Schriftfarbe:"), textColorButton);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    TextStyleDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (!result.accepted) {
        return result;
    }

    result.style.fontFamily = normalizedPdfFontFamily(fontFamilyBox->currentText());
    result.style.fontSize = fontSizeSpin->value();
    result.style.textColor = textColor;
    return result;
}

PasswordDialogResult promptForPasswords(
    QWidget *parent,
    const QString &title,
    const QString &description,
    bool askOwnerPassword,
    bool askUserPassword,
    const QString &ownerLabel,
    const QString &userLabel)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(description, &dialog));

    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    QLineEdit *ownerEdit = nullptr;
    QLineEdit *userEdit = nullptr;

    if (askOwnerPassword) {
        ownerEdit = new QLineEdit(&dialog);
        ownerEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow(ownerLabel, ownerEdit);
    }

    if (askUserPassword) {
        userEdit = new QLineEdit(&dialog);
        userEdit->setEchoMode(QLineEdit::Password);
        formLayout->addRow(userLabel, userEdit);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    PasswordDialogResult result;
    result.accepted = (dialog.exec() == QDialog::Accepted);
    if (result.accepted) {
        if (ownerEdit) {
            result.ownerPassword = ownerEdit->text();
        }
        if (userEdit) {
            result.userPassword = userEdit->text();
        }
    }
    return result;
}

bool cryptographicSigningAvailable()
{
    return !Poppler::availableCryptoSignBackends().isEmpty();
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_documentController(std::make_unique<PdfDocumentController>(std::make_unique<PopplerAdapter>()))
    , m_pdfOperations(std::make_unique<QPdfOperations>())
{
    createActions();
    createCentralLayout();
    createInspectorDock();
    createToolbar();
    createMenus();
    createStatusBar();
    m_themeMode = loadThemeSetting();
    applyTheme(m_themeMode);

    setMinimumSize(1280, 860);

    connect(m_documentController.get(), &PdfDocumentController::pageImageChanged,
            m_pdfView, &PdfView::setPageImage);
    connect(m_documentController.get(), &PdfDocumentController::selectionHighlightChanged,
            m_pdfView, &PdfView::setSelectionHighlights);
    connect(m_documentController.get(), &PdfDocumentController::searchHighlightChanged,
            m_pdfView, &PdfView::setSearchHighlights);
    connect(m_documentController.get(), &PdfDocumentController::annotationOverlaysChanged,
            m_pdfView, &PdfView::setAnnotationOverlays);
    connect(m_documentController.get(), &PdfDocumentController::formFieldOverlaysChanged,
            m_pdfView, &PdfView::setFormFieldOverlays);
    connect(m_documentController.get(), &PdfDocumentController::redactionOverlaysChanged,
            m_pdfView, &PdfView::setRedactionOverlays);
    connect(m_documentController.get(), &PdfDocumentController::zoomChanged,
            this, &MainWindow::updateZoomLabel);
    connect(m_documentController.get(), &PdfDocumentController::currentPageChanged,
            this, &MainWindow::updateCurrentPage);
    connect(m_documentController.get(), &PdfDocumentController::documentStateChanged,
            this, &MainWindow::updateUiState);
    connect(m_documentController.get(), &PdfDocumentController::textSelectionChanged,
            this, &MainWindow::updateSelectionDependentActions);
    connect(m_documentController.get(), &PdfDocumentController::overlaySelectionChanged,
            this, &MainWindow::updateOverlaySelectionActions);
    connect(m_documentController.get(), &PdfDocumentController::searchStateChanged,
            this, &MainWindow::updateSearchState);
    connect(m_documentController.get(), &PdfDocumentController::historyStateChanged,
            this, &MainWindow::updateHistoryActions);
    connect(m_documentController.get(), &PdfDocumentController::errorOccurred,
            this, &MainWindow::showError);
    connect(m_documentController.get(), &PdfDocumentController::statusMessageChanged,
            this, [this](const QString &message) {
                statusBar()->showMessage(message, 4000);
            });
    connect(m_documentController.get(), &PdfDocumentController::ocrFinished,
            this, &MainWindow::showOcrResult);
    connect(m_documentController.get(), &PdfDocumentController::busyStateChanged,
            this, [this](bool busy, const QString &message) {
                if (busy) {
                    QApplication::setOverrideCursor(Qt::WaitCursor);
                    if (!message.isEmpty()) {
                        statusBar()->showMessage(message);
                    }
                } else {
                    QApplication::restoreOverrideCursor();
                    if (!message.isEmpty()) {
                        statusBar()->showMessage(message, 3000);
                    }
                }
            });

    connect(m_pdfView, &PdfView::selectionRectChanged,
            m_documentController.get(), &PdfDocumentController::updateTextSelection);
    connect(m_pdfView, &PdfView::selectionCleared,
            m_documentController.get(), &PdfDocumentController::clearTextSelection);
    connect(m_pdfView, &PdfView::copyRequested,
            m_documentController.get(), &PdfDocumentController::copySelectedText);
    connect(m_pdfView, &PdfView::deleteRequested,
            m_documentController.get(), &PdfDocumentController::deleteSelectedOverlay);
    connect(m_pdfView, &PdfView::nextPageRequested,
            m_documentController.get(), &PdfDocumentController::nextPage);
    connect(m_pdfView, &PdfView::previousPageRequested,
            m_documentController.get(), &PdfDocumentController::previousPage);
    connect(m_pdfView, &PdfView::zoomInRequested,
            m_documentController.get(), &PdfDocumentController::zoomIn);
    connect(m_pdfView, &PdfView::zoomOutRequested,
            m_documentController.get(), &PdfDocumentController::zoomOut);
    connect(m_pdfView, &PdfView::pointActivated,
            m_documentController.get(), &PdfDocumentController::selectOverlayAt);
    connect(m_pdfView, &PdfView::signatureMoveRequested,
            m_documentController.get(), &PdfDocumentController::moveSelectedSignatureBy);
    connect(m_pdfView, &PdfView::signatureResizeRequested,
            m_documentController.get(), &PdfDocumentController::resizeSelectedSignatureBy);
    connect(m_pdfView, &PdfView::textEditResizeRequested,
            m_documentController.get(), &PdfDocumentController::resizeSelectedTextEditBy);
    connect(m_pdfView, &PdfView::contextMenuRequested,
            this, &MainWindow::showPdfViewContextMenu);
    connect(m_pdfView, &PdfView::formTextEdited,
            m_documentController.get(), &PdfDocumentController::setFormFieldText);
    connect(m_pdfView, &PdfView::formCheckToggled,
            m_documentController.get(), &PdfDocumentController::setFormFieldChecked);

    updateWindowTitle();
    updateZoomLabel(m_documentController->zoomFactor());
    updateCurrentPage(0, 0, QString());
    updateSearchState(false, 0, 0, QStringLiteral("Keine aktive Suche."));
    updateUiState(false);
    updateSelectionDependentActions(false);
    updateOverlaySelectionActions(false);
    updateHistoryActions(false, false);
    refreshInspector();
    m_documentController->setThumbnailSize(QSize(140, 180));
}

MainWindow::~MainWindow() = default;

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

void MainWindow::triggerSearch()
{
    m_documentController->setSearchQuery(m_searchEdit->text());
}

void MainWindow::showMetadataDialog()
{
    const PdfDocumentMetadata metadata = m_documentController->metadata();

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Dokument-Metadaten"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    formLayout->addRow(QStringLiteral("Titel:"), new QLabel(metadata.title.isEmpty() ? QStringLiteral("-") : metadata.title, &dialog));
    formLayout->addRow(QStringLiteral("Autor:"), new QLabel(metadata.author.isEmpty() ? QStringLiteral("-") : metadata.author, &dialog));
    formLayout->addRow(QStringLiteral("Betreff:"), new QLabel(metadata.subject.isEmpty() ? QStringLiteral("-") : metadata.subject, &dialog));
    formLayout->addRow(QStringLiteral("Creator:"), new QLabel(metadata.creator.isEmpty() ? QStringLiteral("-") : metadata.creator, &dialog));
    formLayout->addRow(QStringLiteral("Producer:"), new QLabel(metadata.producer.isEmpty() ? QStringLiteral("-") : metadata.producer, &dialog));
    formLayout->addRow(QStringLiteral("Datei:"), new QLabel(metadata.filePath.isEmpty() ? QStringLiteral("-") : metadata.filePath, &dialog));
    formLayout->addRow(QStringLiteral("Seiten:"), new QLabel(QString::number(metadata.pageCount), &dialog));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
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

void MainWindow::showOcrResult(const QString &title, const QString &text)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(720, 520);

    auto *layout = new QVBoxLayout(&dialog);
    auto *textEdit = new QTextEdit(&dialog);
    textEdit->setPlainText(text);
    layout->addWidget(textEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *copyButton = buttons->addButton(QStringLiteral("In Zwischenablage"), QDialogButtonBox::ActionRole);
    connect(copyButton, &QPushButton::clicked, this, [text]() {
        if (QClipboard *clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(text, QClipboard::Clipboard);
            clipboard->setText(text, QClipboard::Selection);
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
}

void MainWindow::updateHistoryActions(bool canUndo, bool canRedo)
{
    if (m_undoAction) {
        m_undoAction->setEnabled(canUndo);
    }
    if (m_redoAction) {
        m_redoAction->setEnabled(canRedo);
    }
}

void MainWindow::requestNoteAnnotation()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const QString noteText = QInputDialog::getMultiLineText(
        this,
        QStringLiteral("Textnotiz"),
        QStringLiteral("Kommentar:"));

    if (noteText.trimmed().isEmpty()) {
        return;
    }

    m_documentController->addNoteAnnotationAt(m_lastContextImagePoint, noteText);
}

void MainWindow::changeSelectedAnnotationColor()
{
    const QColor color = QColorDialog::getColor(QColor(255, 235, 59, 140), this, QStringLiteral("Annotierungsfarbe wählen"));
    if (!color.isValid()) {
        return;
    }

    m_documentController->setSelectedAnnotationColor(color);
}

void MainWindow::editSelectedNoteAnnotation()
{
    if (!m_documentController->hasSelectedNoteAnnotation()) {
        return;
    }

    const QString noteText = QInputDialog::getMultiLineText(
        this,
        QStringLiteral("Textnotiz bearbeiten"),
        QStringLiteral("Kommentar:"),
        m_documentController->selectedNoteText());

    if (noteText.trimmed().isEmpty()) {
        return;
    }

    m_documentController->updateSelectedNoteText(noteText);
}

void MainWindow::requestFreeTextAnnotation()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    const TextAnnotationDialogResult result = promptForTextAnnotation(
        this,
        QStringLiteral("Textfeld hinzufügen"),
        QStringLiteral("Text:"));

    if (!result.accepted || result.text.isEmpty()) {
        return;
    }

    m_documentController->addFreeTextAt(m_lastContextImagePoint, result.text, result.style, result.backgroundColor);
}

void MainWindow::replaceSelectedText()
{
    if (!m_documentController->hasAreaSelection()) {
        return;
    }

    const TextAnnotationDialogResult result = promptForTextAnnotation(
        this,
        QStringLiteral("Text ersetzen"),
        QStringLiteral("Neuer Text:"),
        m_documentController->selectedText());

    if (!result.accepted || result.text.isEmpty()) {
        return;
    }

    m_documentController->replaceSelectedText(result.text, result.style, result.backgroundColor);
}

void MainWindow::editSelectedTextAnnotation()
{
    if (!m_documentController->hasSelectedTextEditAnnotation()) {
        return;
    }

    const TextAnnotationDialogResult result = promptForTextAnnotation(
        this,
        QStringLiteral("Textfeld bearbeiten"),
        QStringLiteral("Text:"),
        m_documentController->selectedTextEditText(),
        m_documentController->selectedTextEditStyle(),
        m_documentController->selectedTextEditBackgroundColor());

    if (!result.accepted || result.text.isEmpty()) {
        return;
    }

    m_documentController->updateSelectedTextEdit(result.text, result.style, result.backgroundColor);
}

void MainWindow::editFormFieldTextStyle()
{
    QString fieldId;
    QString fieldLabel;
    PdfTextStyle style;
    if (!m_documentController->textFormFieldStyleAt(m_lastContextImagePoint, fieldId, fieldLabel, style)) {
        return;
    }

    const TextStyleDialogResult result = promptForTextStyle(
        this,
        fieldLabel.trimmed().isEmpty()
            ? QStringLiteral("Formularschrift anpassen")
            : QStringLiteral("Formularschrift anpassen: %1").arg(fieldLabel),
        style);

    if (!result.accepted) {
        return;
    }

    m_documentController->setFormFieldTextStyle(fieldId, result.style);
}

void MainWindow::updateWindowTitle()
{
    const QString title = m_documentController->hasDocument()
        ? QStringLiteral("PDF Tool - %1").arg(m_documentController->documentPath())
        : QStringLiteral("PDF Tool");
    setWindowTitle(title);
    if (m_documentStatusLabel) {
        m_documentStatusLabel->setText(
            m_documentController->hasDocument()
                ? QFileInfo(m_documentController->documentPath()).fileName()
                : QStringLiteral("Kein Dokument"));
    }
}

void MainWindow::updateZoomLabel(double factor)
{
    m_zoomLabel->setText(QStringLiteral("Zoom: %1%").arg(qRound(factor * 100.0)));
}

void MainWindow::updateCurrentPage(int pageIndex, int pageCount, const QString &pageLabel)
{
    if (pageCount <= 0) {
        m_pageStatusLabel->setText(QStringLiteral("Seite: -/-"));
        if (m_pageJumpSpinBox) {
            const QSignalBlocker blocker(m_pageJumpSpinBox);
            m_pageJumpSpinBox->setMinimum(0);
            m_pageJumpSpinBox->setMaximum(0);
            m_pageJumpSpinBox->setValue(0);
            m_pageJumpSpinBox->setEnabled(false);
        }
        if (m_pageCountHintLabel) {
            m_pageCountHintLabel->setText(QStringLiteral("Keine Seiten"));
        }
        if (m_modeStatusLabel) {
            m_modeStatusLabel->setText(QStringLiteral("Bereit"));
        }
        return;
    }

    m_pageStatusLabel->setText(QStringLiteral("Seite: %1/%2 (%3)")
        .arg(pageIndex + 1)
        .arg(pageCount)
        .arg(pageLabel));
    if (m_pageJumpSpinBox) {
        const QSignalBlocker blocker(m_pageJumpSpinBox);
        m_pageJumpSpinBox->setEnabled(true);
        m_pageJumpSpinBox->setMinimum(1);
        m_pageJumpSpinBox->setMaximum(pageCount);
        m_pageJumpSpinBox->setValue(pageIndex + 1);
    }
    if (m_pageCountHintLabel) {
        m_pageCountHintLabel->setText(QStringLiteral("von %1").arg(pageCount));
    }
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText(QStringLiteral("Lesen"));
    }

    syncThumbnailSelection(pageIndex);
}

void MainWindow::updateUiState(bool hasDocument)
{
    updateHistoryActions(m_documentController->canUndo(), m_documentController->canRedo());
    m_saveAction->setEnabled(hasDocument);
    m_mergePdfsAction->setEnabled(m_pdfOperations->isAvailable());
    m_splitPdfAction->setEnabled(hasDocument && m_pdfOperations->isAvailable());
    m_exportEncryptedPdfAction->setEnabled(hasDocument && m_pdfOperations->isAvailable());
    m_exportEditedPdfAction->setEnabled(hasDocument);
    m_exportRedactedPdfAction->setEnabled(hasDocument && m_documentController->hasRedactions());
    m_exportSignedPdfAction->setEnabled(
        hasDocument
        && cryptographicSigningAvailable()
        && m_documentController->hasSelectedSignatureAnnotation());
    m_printAction->setEnabled(hasDocument);
    m_showMetadataAction->setEnabled(hasDocument);
    m_zoomInAction->setEnabled(hasDocument);
    m_zoomOutAction->setEnabled(hasDocument);
    m_resetZoomAction->setEnabled(hasDocument);
    m_previousPageAction->setEnabled(hasDocument);
    m_nextPageAction->setEnabled(hasDocument);
    m_findAction->setEnabled(hasDocument);
    m_clearSearchAction->setEnabled(hasDocument && m_documentController->hasSearchResults());
    m_searchEdit->setEnabled(hasDocument);
    if (m_searchButton) {
        m_searchButton->setEnabled(hasDocument);
    }
    if (m_pageJumpSpinBox && !hasDocument) {
        m_pageJumpSpinBox->setEnabled(false);
    }
    m_insertSignatureAction->setEnabled(hasDocument);
    m_runOcrCurrentPageAction->setEnabled(hasDocument && m_documentController->isOcrAvailable());
    m_runOcrSelectionAction->setEnabled(hasDocument && m_documentController->isOcrAvailable()
                                        && m_documentController->hasAreaSelection());
    m_movePageLeftAction->setEnabled(hasDocument && m_pdfOperations->isAvailable() && m_documentController->currentPageIndex() > 0);
    m_movePageRightAction->setEnabled(
        hasDocument
        && m_pdfOperations->isAvailable()
        && m_documentController->currentPageIndex() >= 0
        && m_documentController->currentPageIndex() < m_documentController->pageCount() - 1);
    m_deletePageAction->setEnabled(hasDocument && m_pdfOperations->isAvailable() && m_documentController->pageCount() > 1);
    m_rectangleAnnotationAction->setEnabled(hasDocument);
    m_noteAnnotationAction->setEnabled(hasDocument);
    m_addTextAction->setEnabled(hasDocument);
    m_redactSelectionAction->setEnabled(hasDocument);
    m_replaceSelectedTextAction->setEnabled(hasDocument && m_documentController->hasAreaSelection());

    if (!hasDocument) {
        updateHistoryActions(false, false);
        if (m_documentStatusLabel) {
            m_documentStatusLabel->setText(QStringLiteral("Kein Dokument"));
        }
        if (m_modeStatusLabel) {
            m_modeStatusLabel->setText(QStringLiteral("Bereit"));
        }
        m_copyAction->setEnabled(false);
        m_highlightSelectionAction->setEnabled(false);
        m_deleteOverlayAction->setEnabled(false);
        m_changeAnnotationColorAction->setEnabled(false);
        m_editNoteAnnotationAction->setEnabled(false);
        m_editTextAction->setEnabled(false);
        m_findNextAction->setEnabled(false);
        m_findPreviousAction->setEnabled(false);
        syncThumbnailSelection(-1);
        m_outlineTreeWidget->clear();
        updateCurrentPage(0, 0, QString());
    }
}

void MainWindow::updateSelectionDependentActions(bool hasSelection)
{
    m_copyAction->setEnabled(hasSelection);
    m_highlightSelectionAction->setEnabled(hasSelection);
    m_replaceSelectedTextAction->setEnabled(hasSelection || m_documentController->hasAreaSelection());
    m_runOcrSelectionAction->setEnabled(m_documentController->hasDocument()
                                        && m_documentController->isOcrAvailable()
                                        && m_documentController->hasAreaSelection());
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText(hasSelection ? QStringLiteral("Auswahl aktiv") : QStringLiteral("Lesen"));
    }
}

void MainWindow::updateOverlaySelectionActions(bool hasSelection)
{
    m_deleteOverlayAction->setEnabled(hasSelection);
    m_changeAnnotationColorAction->setEnabled(hasSelection);
    m_editNoteAnnotationAction->setEnabled(hasSelection && m_documentController->hasSelectedNoteAnnotation());
    m_editTextAction->setEnabled(hasSelection && m_documentController->hasSelectedTextEditAnnotation());
    m_exportEditedPdfAction->setEnabled(m_documentController->hasDocument());
    m_exportRedactedPdfAction->setEnabled(m_documentController->hasDocument() && m_documentController->hasRedactions());
    m_exportSignedPdfAction->setEnabled(
        m_documentController->hasDocument()
        && cryptographicSigningAvailable()
        && m_documentController->hasSelectedSignatureAnnotation());
    if (m_modeStatusLabel && hasSelection) {
        m_modeStatusLabel->setText(QStringLiteral("Objekt ausgewählt"));
    }
    refreshInspector();
}

void MainWindow::updateSearchState(bool hasResults, int currentHit, int totalHits, const QString &statusText)
{
    m_searchStatusLabel->setText(hasResults
        ? QStringLiteral("Treffer %1 / %2").arg(currentHit).arg(totalHits)
        : statusText);
    m_findNextAction->setEnabled(hasResults);
    m_findPreviousAction->setEnabled(hasResults);
    m_clearSearchAction->setEnabled(m_documentController->hasDocument());
}

void MainWindow::jumpToPage(int pageNumber)
{
    if (pageNumber > 0) {
        m_documentController->goToPage(pageNumber - 1);
    }
}

void MainWindow::moveCurrentPageLeft()
{
    if (!m_documentController->hasDocument() || m_documentController->pageCount() < 2) {
        return;
    }

    const int currentIndex = m_documentController->currentPageIndex();
    if (currentIndex <= 0) {
        return;
    }

    QVector<int> newOrder;
    newOrder.reserve(m_documentController->pageCount());
    for (int index = 0; index < m_documentController->pageCount(); ++index) {
        newOrder.append(index);
    }
    newOrder.swapItemsAt(currentIndex, currentIndex - 1);

    applyPageOrderChange(newOrder, currentIndex - 1, QStringLiteral("Seite nach links verschoben."));
}

void MainWindow::moveCurrentPageRight()
{
    if (!m_documentController->hasDocument() || m_documentController->pageCount() < 2) {
        return;
    }

    const int currentIndex = m_documentController->currentPageIndex();
    if (currentIndex >= m_documentController->pageCount() - 1) {
        return;
    }

    QVector<int> newOrder;
    newOrder.reserve(m_documentController->pageCount());
    for (int index = 0; index < m_documentController->pageCount(); ++index) {
        newOrder.append(index);
    }
    newOrder.swapItemsAt(currentIndex, currentIndex + 1);

    applyPageOrderChange(newOrder, currentIndex + 1, QStringLiteral("Seite nach rechts verschoben."));
}

void MainWindow::deleteCurrentPage()
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    if (m_documentController->pageCount() <= 1) {
        showError(QStringLiteral("Die letzte Seite kann nicht gelöscht werden."));
        return;
    }

    const int currentIndex = m_documentController->currentPageIndex();
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Seite löschen"),
        QStringLiteral("Aktuelle Seite wirklich löschen?"));
    if (answer != QMessageBox::Yes) {
        return;
    }

    QVector<int> newOrder;
    newOrder.reserve(m_documentController->pageCount() - 1);
    for (int index = 0; index < m_documentController->pageCount(); ++index) {
        if (index != currentIndex) {
            newOrder.append(index);
        }
    }

    applyPageOrderChange(
        newOrder,
        qMin(currentIndex, m_documentController->pageCount() - 2),
        QStringLiteral("Seite gelöscht."));
}

void MainWindow::setDarkMode(bool enabled)
{
    const ThemeMode newMode = enabled ? ThemeMode::Dark : ThemeMode::Light;
    if (m_themeMode == newMode) {
        return;
    }

    m_themeMode = newMode;
    applyTheme(m_themeMode);
    saveThemeSetting(m_themeMode);
}

void MainWindow::applyWindowStyle()
{
    applyTheme(m_themeMode);
}

void MainWindow::applyTheme(ThemeMode mode)
{
    setDockOptions(QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks);

    if (mode == ThemeMode::Dark) {
        setStyleSheet(QStringLiteral(
            "QMainWindow, QMenuBar, QMenu, QStatusBar {"
            "  background: #12171c;"
            "  color: #e3e8eb;"
            "}"
            "QMenuBar {"
            "  border-bottom: 1px solid #28323a;"
            "  padding: 4px 8px;"
            "}"
            "QMenuBar::item {"
            "  padding: 6px 10px;"
            "  border-radius: 6px;"
            "}"
            "QMenuBar::item:selected, QMenu::item:selected {"
            "  background: #24313a;"
            "}"
            "QMenu {"
            "  border: 1px solid #2d3942;"
            "  padding: 6px;"
            "}"
            "QToolBar {"
            "  background: #171d23;"
            "  border: 0;"
            "  border-bottom: 1px solid #29343d;"
            "  spacing: 3px;"
            "  padding: 4px 8px;"
            "}"
            "QToolButton {"
            "  color: #e3e8eb;"
            "  background: transparent;"
            "  border: 1px solid transparent;"
            "  border-radius: 8px;"
            "  padding: 5px 7px;"
            "  margin: 0;"
            "}"
            "QToolButton:hover {"
            "  background: #22303a;"
            "  border-color: #314552;"
            "}"
            "QToolButton:pressed, QToolButton:checked {"
            "  background: #2b4351;"
            "  border-color: #4b7183;"
            "}"
            "QDockWidget {"
            "  font-weight: 600;"
            "  color: #dfe6ea;"
            "}"
            "QDockWidget::title {"
            "  text-align: left;"
            "  background: #1b232b;"
            "  padding: 10px 12px;"
            "  border-bottom: 1px solid #2c3841;"
            "}"
            "QTabWidget::pane {"
            "  border: 1px solid #2c3841;"
            "  background: #171d23;"
            "  border-radius: 10px;"
            "}"
            "QTabBar::tab {"
            "  background: #202932;"
            "  border: 1px solid #32404a;"
            "  border-bottom: none;"
            "  padding: 9px 16px;"
            "  margin-right: 4px;"
            "  border-top-left-radius: 8px;"
            "  border-top-right-radius: 8px;"
            "}"
            "QTabBar::tab:selected {"
            "  background: #171d23;"
            "}"
            "QTabBar::tab:hover {"
            "  background: #26323c;"
            "}"
            "QListView, QTreeWidget {"
            "  background: #171d23;"
            "  color: #dfe6ea;"
            "  border: none;"
            "  outline: none;"
            "  padding: 10px;"
            "}"
            "QListView::item, QTreeWidget::item {"
            "  border-radius: 10px;"
            "}"
            "QListView::item:selected, QTreeWidget::item:selected {"
            "  background: #29414d;"
            "  color: #f1f5f7;"
            "}"
            "QListView::item:hover, QTreeWidget::item:hover {"
            "  background: #1f2a32;"
            "}"
            "QLineEdit {"
            "  color: #eef3f6;"
            "  background: #0f1418;"
            "  border: 1px solid #34434d;"
            "  border-radius: 9px;"
            "  padding: 8px 12px;"
            "  selection-background-color: #0f6c73;"
            "}"
            "QSpinBox {"
            "  color: #eef3f6;"
            "  background: #0f1418;"
            "  border: 1px solid #34434d;"
            "  border-radius: 9px;"
            "  padding: 6px 8px;"
            "}"
            "QPushButton {"
            "  color: #eef3f6;"
            "  background: #24414f;"
            "  border: 1px solid #356173;"
            "  border-radius: 9px;"
            "  padding: 7px 12px;"
            "}"
            "QPushButton:hover {"
            "  background: #2d5263;"
            "}"
            "QStatusBar {"
            "  border-top: 1px solid #28323a;"
            "}"
            "QStatusBar::item {"
            "  border: none;"
            "}"
            "QLabel#statusPill {"
            "  background: #1c252c;"
            "  color: #dce3e7;"
            "  border: 1px solid #34424a;"
            "  border-radius: 10px;"
            "  padding: 6px 10px;"
            "}"
            "QLabel#statusPillAccent {"
            "  background: #143139;"
            "  color: #9ed8dd;"
            "  border: 1px solid #22525b;"
            "  border-radius: 10px;"
            "  padding: 6px 10px;"
            "}"));
    } else {
        setStyleSheet(QStringLiteral(
            "QMainWindow, QMenuBar, QMenu, QStatusBar {"
            "  background: #f5f1e8;"
            "  color: #1e2b2f;"
            "}"
            "QMenuBar {"
            "  border-bottom: 1px solid #d8d0c0;"
            "  padding: 4px 8px;"
            "}"
            "QMenuBar::item {"
            "  padding: 6px 10px;"
            "  border-radius: 6px;"
            "}"
            "QMenuBar::item:selected, QMenu::item:selected {"
            "  background: #dfe9e4;"
            "}"
            "QMenu {"
            "  border: 1px solid #d8d0c0;"
            "  padding: 6px;"
            "}"
            "QToolBar {"
            "  background: #fbf8f2;"
            "  border: 0;"
            "  border-bottom: 1px solid #d8d0c0;"
            "  spacing: 3px;"
            "  padding: 4px 8px;"
            "}"
            "QToolButton {"
            "  background: transparent;"
            "  border: 1px solid transparent;"
            "  border-radius: 8px;"
            "  padding: 5px 7px;"
            "  margin: 0;"
            "}"
            "QToolButton:hover {"
            "  background: #e8efe9;"
            "  border-color: #c2d0c9;"
            "}"
            "QToolButton:pressed, QToolButton:checked {"
            "  background: #d4e4db;"
            "  border-color: #8ca99b;"
            "}"
            "QDockWidget {"
            "  font-weight: 600;"
            "  color: #24353a;"
            "}"
            "QDockWidget::title {"
            "  text-align: left;"
            "  background: #e9e1d3;"
            "  padding: 10px 12px;"
            "  border-bottom: 1px solid #d8d0c0;"
            "}"
            "QTabWidget::pane {"
            "  border: 1px solid #d8d0c0;"
            "  background: #fbf8f2;"
            "  border-radius: 10px;"
            "}"
            "QTabBar::tab {"
            "  background: #ede6d8;"
            "  border: 1px solid #d8d0c0;"
            "  border-bottom: none;"
            "  padding: 9px 16px;"
            "  margin-right: 4px;"
            "  border-top-left-radius: 8px;"
            "  border-top-right-radius: 8px;"
            "}"
            "QTabBar::tab:selected {"
            "  background: #fbf8f2;"
            "}"
            "QTabBar::tab:hover {"
            "  background: #e2dbce;"
            "}"
            "QListView, QTreeWidget {"
            "  background: #fbf8f2;"
            "  border: none;"
            "  outline: none;"
            "  padding: 10px;"
            "}"
            "QListView::item, QTreeWidget::item {"
            "  border-radius: 10px;"
            "}"
            "QListView::item:selected, QTreeWidget::item:selected {"
            "  background: #d9e7e0;"
            "  color: #1f2b2f;"
            "}"
            "QListView::item:hover, QTreeWidget::item:hover {"
            "  background: #edf3ef;"
            "}"
            "QLineEdit {"
            "  background: white;"
            "  border: 1px solid #cbbfae;"
            "  border-radius: 9px;"
            "  padding: 8px 12px;"
            "  selection-background-color: #0f6c73;"
            "}"
            "QSpinBox {"
            "  background: white;"
            "  border: 1px solid #cbbfae;"
            "  border-radius: 9px;"
            "  padding: 6px 8px;"
            "}"
            "QPushButton {"
            "  background: #dce9e3;"
            "  color: #0f4b52;"
            "  border: 1px solid #b8cfc4;"
            "  border-radius: 9px;"
            "  padding: 7px 12px;"
            "}"
            "QPushButton:hover {"
            "  background: #cfe1d9;"
            "}"
            "QStatusBar {"
            "  border-top: 1px solid #d8d0c0;"
            "}"
            "QStatusBar::item {"
            "  border: none;"
            "}"
            "QLabel#statusPill {"
            "  background: #ece5d8;"
            "  color: #24353a;"
            "  border: 1px solid #d0c5b5;"
            "  border-radius: 10px;"
            "  padding: 6px 10px;"
            "}"
            "QLabel#statusPillAccent {"
            "  background: #dce9e3;"
            "  color: #0f4b52;"
            "  border: 1px solid #b8cfc4;"
            "  border-radius: 10px;"
            "  padding: 6px 10px;"
            "}"));
    }

    if (m_pdfView) {
        m_pdfView->setDarkMode(mode == ThemeMode::Dark);
    }
    if (m_toggleDarkModeAction) {
        const QSignalBlocker blocker(m_toggleDarkModeAction);
        m_toggleDarkModeAction->setChecked(mode == ThemeMode::Dark);
    }
}

MainWindow::ThemeMode MainWindow::loadThemeSetting() const
{
    const QSettings settings(QStringLiteral("PDFTool"), QStringLiteral("PDFTool"));
    return settings.value(QStringLiteral("ui/theme"), QStringLiteral("light")).toString() == QStringLiteral("dark")
        ? ThemeMode::Dark
        : ThemeMode::Light;
}

void MainWindow::saveThemeSetting(ThemeMode mode) const
{
    QSettings settings(QStringLiteral("PDFTool"), QStringLiteral("PDFTool"));
    settings.setValue(QStringLiteral("ui/theme"), mode == ThemeMode::Dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

QWidget *MainWindow::createToolbarSpacer(int width) const
{
    auto *spacer = new QWidget();
    spacer->setFixedWidth(width);
    return spacer;
}

QLabel *MainWindow::createStatusPill(const QString &text, const QString &objectName) const
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName);
    label->setMinimumHeight(30);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

bool MainWindow::applyPageOrderChange(const QVector<int> &newOrder, int reopenedPageIndex, const QString &successMessage)
{
    if (!m_documentController->hasDocument() || !m_pdfOperations->isAvailable()) {
        return false;
    }

    const QString documentPath = m_documentController->documentPath();
    const QByteArray ownerPassword = m_documentController->currentOwnerPassword();
    const QByteArray userPassword = m_documentController->currentUserPassword();
    const QString tempPath = documentPath + QStringLiteral(".pageops.tmp.pdf");

    QFile::remove(tempPath);
    if (!m_pdfOperations->reorderPages(documentPath, tempPath, newOrder, userPassword)) {
        showError(m_pdfOperations->lastError());
        return false;
    }

    if (!QFile::remove(documentPath)) {
        QFile::remove(tempPath);
        showError(QStringLiteral("Originaldatei konnte nicht ersetzt werden."));
        return false;
    }

    if (!QFile::rename(tempPath, documentPath)) {
        showError(QStringLiteral("Temporäre PDF-Datei konnte nicht übernommen werden."));
        return false;
    }

    if (!m_documentController->remapPageOrder(newOrder)) {
        showError(QStringLiteral("Begleitdaten konnten nicht auf die neue Seitenreihenfolge angepasst werden."));
        return false;
    }

    if (!m_documentController->openDocument(documentPath, ownerPassword, userPassword)) {
        showError(m_documentController->lastError());
        return false;
    }

    refreshNavigationPanels();
    updateWindowTitle();
    if (reopenedPageIndex >= 0 && reopenedPageIndex < m_documentController->pageCount()) {
        m_documentController->goToPage(reopenedPageIndex);
    }
    statusBar()->showMessage(successMessage, 4000);
    return true;
}

void MainWindow::navigateToSidebarPage(const QModelIndex &index)
{
    if (index.isValid()) {
        m_documentController->goToPage(index.row());
    }
}

void MainWindow::navigateToOutlineItem(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    if (!item) {
        return;
    }

    const int pageIndex = item->data(0, Qt::UserRole).toInt();
    if (pageIndex >= 0) {
        m_documentController->goToPage(pageIndex);
    }
}

void MainWindow::focusSearchField()
{
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, QStringLiteral("Fehler"), message);
}

void MainWindow::showPdfViewContextMenu(const QPointF &imagePoint, const QPoint &globalPos)
{
    if (!m_documentController->hasDocument()) {
        return;
    }

    m_lastContextImagePoint = imagePoint;
    m_documentController->selectOverlayAt(imagePoint);

    QMenu menu(this);
    if (m_documentController->hasTextSelection()) {
        menu.addAction(m_highlightSelectionAction);
    }
    if (m_documentController->hasAreaSelection()) {
        menu.addAction(m_replaceSelectedTextAction);
        menu.addAction(m_rectangleAnnotationAction);
        menu.addAction(m_redactSelectionAction);
        if (m_documentController->isOcrAvailable()) {
            menu.addAction(m_runOcrSelectionAction);
        }
    }
    menu.addAction(m_addTextAction);
    menu.addAction(m_insertSignatureAction);
    menu.addAction(m_noteAnnotationAction);
    if (m_documentController->isOcrAvailable()) {
        menu.addAction(m_runOcrCurrentPageAction);
    }
    menu.addSeparator();
    menu.addAction(m_movePageLeftAction);
    menu.addAction(m_movePageRightAction);
    menu.addAction(m_deletePageAction);

    if (m_documentController->hasSelectedOverlay()) {
        menu.addSeparator();
        menu.addAction(m_deleteOverlayAction);
        if (m_documentController->hasSelectedNoteAnnotation()) {
            menu.addAction(m_editNoteAnnotationAction);
        }
        if (m_documentController->hasSelectedTextEditAnnotation()) {
            menu.addAction(m_editTextAction);
        }
        menu.addAction(m_changeAnnotationColorAction);
    }

    QString formFieldId;
    QString formFieldLabel;
    PdfTextStyle formFieldStyle;
    if (m_documentController->textFormFieldStyleAt(imagePoint, formFieldId, formFieldLabel, formFieldStyle)) {
        menu.addSeparator();
        menu.addAction(m_editFormFieldTextStyleAction);
    }

    if (!menu.isEmpty()) {
        menu.exec(globalPos);
    }
}

void MainWindow::createActions()
{
    m_openAction = new QAction(QStringLiteral("Öffnen..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIcon(loadToolbarIcon(QStringLiteral("open")));
    m_openAction->setToolTip(QStringLiteral("PDF öffnen (Strg+O)"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openPdf);

    m_saveAction = new QAction(QStringLiteral("Speichern"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setIcon(loadToolbarIcon(QStringLiteral("save")));
    m_saveAction->setToolTip(QStringLiteral("Änderungen speichern (Strg+S)"));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveDocument);

    m_printAction = new QAction(QStringLiteral("Drucken..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setIcon(loadToolbarIcon(QStringLiteral("print")));
    m_printAction->setToolTip(QStringLiteral("Dokument drucken (Strg+P)"));
    connect(m_printAction, &QAction::triggered, this, &MainWindow::printDocument);

    m_mergePdfsAction = new QAction(QStringLiteral("PDFs zusammenführen..."), this);
    connect(m_mergePdfsAction, &QAction::triggered, this, &MainWindow::mergePdfFiles);

    m_splitPdfAction = new QAction(QStringLiteral("Dokument teilen..."), this);
    connect(m_splitPdfAction, &QAction::triggered, this, &MainWindow::splitCurrentPdf);

    m_exportEncryptedPdfAction = new QAction(QStringLiteral("Als verschlüsseltes PDF exportieren..."), this);
    connect(m_exportEncryptedPdfAction, &QAction::triggered, this, &MainWindow::exportEncryptedPdf);

    m_exportEditedPdfAction = new QAction(QStringLiteral("Bearbeitetes PDF exportieren..."), this);
    connect(m_exportEditedPdfAction, &QAction::triggered, this, &MainWindow::exportEditedPdf);

    m_exportRedactedPdfAction = new QAction(QStringLiteral("Geschwärztes PDF exportieren..."), this);
    connect(m_exportRedactedPdfAction, &QAction::triggered, this, &MainWindow::exportRedactedPdf);

    m_exportSignedPdfAction = new QAction(QStringLiteral("Kryptografisch signiertes PDF exportieren..."), this);
    connect(m_exportSignedPdfAction, &QAction::triggered, this, &MainWindow::exportSignedPdf);

    m_showMetadataAction = new QAction(QStringLiteral("Metadaten"), this);
    m_showMetadataAction->setIcon(loadToolbarIcon(QStringLiteral("info")));
    m_showMetadataAction->setToolTip(QStringLiteral("Dokument-Metadaten anzeigen"));
    connect(m_showMetadataAction, &QAction::triggered, this, &MainWindow::showMetadataDialog);

    m_exitAction = new QAction(QStringLiteral("Beenden"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    m_zoomInAction = new QAction(QStringLiteral("Zoom +"), this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAction->setIcon(loadToolbarIcon(QStringLiteral("zoom_in")));
    m_zoomInAction->setToolTip(QStringLiteral("Vergrößern (%1)").arg(QKeySequence(QKeySequence::ZoomIn).toString(QKeySequence::NativeText)));
    connect(m_zoomInAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::zoomIn);

    m_zoomOutAction = new QAction(QStringLiteral("Zoom -"), this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAction->setIcon(loadToolbarIcon(QStringLiteral("zoom_out")));
    m_zoomOutAction->setToolTip(QStringLiteral("Verkleinern (%1)").arg(QKeySequence(QKeySequence::ZoomOut).toString(QKeySequence::NativeText)));
    connect(m_zoomOutAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::zoomOut);

    m_resetZoomAction = new QAction(QStringLiteral("100%"), this);
    m_resetZoomAction->setIcon(loadToolbarIcon(QStringLiteral("zoom_reset")));
    m_resetZoomAction->setToolTip(QStringLiteral("Zoom auf 100% zurücksetzen"));
    connect(m_resetZoomAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::resetZoom);

    m_previousPageAction = new QAction(QStringLiteral("Vorherige Seite"), this);
    m_previousPageAction->setShortcut(QKeySequence(Qt::Key_PageUp));
    m_previousPageAction->setIcon(loadToolbarIcon(QStringLiteral("previous")));
    m_previousPageAction->setToolTip(QStringLiteral("Vorherige Seite (Bild auf)"));
    connect(m_previousPageAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::previousPage);

    m_nextPageAction = new QAction(QStringLiteral("Nächste Seite"), this);
    m_nextPageAction->setShortcut(QKeySequence(Qt::Key_PageDown));
    m_nextPageAction->setIcon(loadToolbarIcon(QStringLiteral("next")));
    m_nextPageAction->setToolTip(QStringLiteral("Nächste Seite (Bild ab)"));
    connect(m_nextPageAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::nextPage);

    m_copyAction = new QAction(QStringLiteral("Kopieren"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(m_copyAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::copySelectedText);

    m_undoAction = new QAction(QStringLiteral("Rückgängig"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(loadToolbarIcon(QStringLiteral("undo")));
    m_undoAction->setToolTip(QStringLiteral("Rückgängig (%1)").arg(QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText)));
    connect(m_undoAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::undo);

    m_redoAction = new QAction(QStringLiteral("Wiederholen"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setIcon(loadToolbarIcon(QStringLiteral("redo")));
    m_redoAction->setToolTip(QStringLiteral("Wiederholen (%1)").arg(QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText)));
    connect(m_redoAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::redo);

    m_deleteOverlayAction = new QAction(QStringLiteral("Löschen"), this);
    m_deleteOverlayAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_deleteOverlayAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    connect(m_deleteOverlayAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::deleteSelectedOverlay);

    m_highlightSelectionAction = new QAction(QStringLiteral("Text hervorheben"), this);
    connect(m_highlightSelectionAction, &QAction::triggered,
            m_documentController.get(), &PdfDocumentController::addHighlightAnnotationFromSelection);

    m_rectangleAnnotationAction = new QAction(QStringLiteral("Rechteck-Annotation"), this);
    connect(m_rectangleAnnotationAction, &QAction::triggered,
            m_documentController.get(), &PdfDocumentController::addRectangleAnnotationFromSelection);

    m_noteAnnotationAction = new QAction(QStringLiteral("Textnotiz hinzufügen"), this);
    connect(m_noteAnnotationAction, &QAction::triggered, this, &MainWindow::requestNoteAnnotation);

    m_editNoteAnnotationAction = new QAction(QStringLiteral("Textnotiz bearbeiten"), this);
    connect(m_editNoteAnnotationAction, &QAction::triggered, this, &MainWindow::editSelectedNoteAnnotation);

    m_addTextAction = new QAction(QStringLiteral("Textfeld hinzufügen"), this);
    connect(m_addTextAction, &QAction::triggered, this, &MainWindow::requestFreeTextAnnotation);

    m_replaceSelectedTextAction = new QAction(QStringLiteral("Text ersetzen"), this);
    connect(m_replaceSelectedTextAction, &QAction::triggered, this, &MainWindow::replaceSelectedText);

    m_editTextAction = new QAction(QStringLiteral("Textfeld bearbeiten"), this);
    connect(m_editTextAction, &QAction::triggered, this, &MainWindow::editSelectedTextAnnotation);

    m_editFormFieldTextStyleAction = new QAction(QStringLiteral("Formularschrift anpassen"), this);
    connect(m_editFormFieldTextStyleAction, &QAction::triggered, this, &MainWindow::editFormFieldTextStyle);

    m_changeAnnotationColorAction = new QAction(QStringLiteral("Annotierungsfarbe ändern"), this);
    connect(m_changeAnnotationColorAction, &QAction::triggered, this, &MainWindow::changeSelectedAnnotationColor);

    m_redactSelectionAction = new QAction(QStringLiteral("Schwärzen"), this);
    connect(m_redactSelectionAction, &QAction::triggered,
            m_documentController.get(), &PdfDocumentController::addRedactionFromSelection);

    m_findAction = new QAction(QStringLiteral("Suchen"), this);
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setIcon(loadToolbarIcon(QStringLiteral("search")));
    connect(m_findAction, &QAction::triggered, this, &MainWindow::focusSearchField);

    m_toggleDarkModeAction = new QAction(QStringLiteral("Dunkelmodus"), this);
    m_toggleDarkModeAction->setCheckable(true);
    m_toggleDarkModeAction->setIcon(loadToolbarIcon(QStringLiteral("theme")));
    m_toggleDarkModeAction->setToolTip(QStringLiteral("Dunkelmodus umschalten"));
    connect(m_toggleDarkModeAction, &QAction::toggled, this, &MainWindow::setDarkMode);

    m_insertSignatureAction = new QAction(QStringLiteral("Unterschrift einfügen..."), this);
    connect(m_insertSignatureAction, &QAction::triggered, this, &MainWindow::insertSignature);

    m_runOcrCurrentPageAction = new QAction(QStringLiteral("OCR aktuelle Seite"), this);
    connect(m_runOcrCurrentPageAction, &QAction::triggered, this, &MainWindow::runOcrCurrentPage);

    m_runOcrSelectionAction = new QAction(QStringLiteral("OCR Auswahl"), this);
    connect(m_runOcrSelectionAction, &QAction::triggered, this, &MainWindow::runOcrSelection);

    m_movePageLeftAction = new QAction(QStringLiteral("Seite nach links"), this);
    connect(m_movePageLeftAction, &QAction::triggered, this, &MainWindow::moveCurrentPageLeft);

    m_movePageRightAction = new QAction(QStringLiteral("Seite nach rechts"), this);
    connect(m_movePageRightAction, &QAction::triggered, this, &MainWindow::moveCurrentPageRight);

    m_deletePageAction = new QAction(QStringLiteral("Seite löschen"), this);
    connect(m_deletePageAction, &QAction::triggered, this, &MainWindow::deleteCurrentPage);

    m_findNextAction = new QAction(QStringLiteral("Nächster Treffer"), this);
    m_findNextAction->setShortcut(QKeySequence(Qt::Key_F3));
    m_findNextAction->setIcon(loadToolbarIcon(QStringLiteral("search_next")));
    m_findNextAction->setToolTip(QStringLiteral("Nächster Treffer (F3)"));
    connect(m_findNextAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::findNext);

    m_findPreviousAction = new QAction(QStringLiteral("Vorheriger Treffer"), this);
    m_findPreviousAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3));
    m_findPreviousAction->setIcon(loadToolbarIcon(QStringLiteral("search_previous")));
    m_findPreviousAction->setToolTip(QStringLiteral("Vorheriger Treffer (Umschalt+F3)"));
    connect(m_findPreviousAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::findPrevious);

    m_clearSearchAction = new QAction(QStringLiteral("Suche löschen"), this);
    connect(m_clearSearchAction, &QAction::triggered, m_documentController.get(), &PdfDocumentController::clearSearch);

    addAction(m_saveAction);
    addAction(m_copyAction);
    addAction(m_undoAction);
    addAction(m_redoAction);
    addAction(m_deleteOverlayAction);
    addAction(m_findAction);
    addAction(m_findNextAction);
    addAction(m_findPreviousAction);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("Datei"));
    fileMenu->addAction(m_openAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_printAction);
    fileMenu->addAction(m_mergePdfsAction);
    fileMenu->addAction(m_splitPdfAction);
    fileMenu->addAction(m_exportEncryptedPdfAction);
    fileMenu->addAction(m_exportEditedPdfAction);
    fileMenu->addAction(m_exportRedactedPdfAction);
    fileMenu->addAction(m_exportSignedPdfAction);
    fileMenu->addAction(m_showMetadataAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *editMenu = menuBar()->addMenu(QStringLiteral("Bearbeiten"));
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_deleteOverlayAction);
    editMenu->addSeparator();
    editMenu->addAction(m_insertSignatureAction);
    editMenu->addAction(m_highlightSelectionAction);
    editMenu->addAction(m_replaceSelectedTextAction);
    editMenu->addAction(m_addTextAction);
    editMenu->addAction(m_rectangleAnnotationAction);
    editMenu->addAction(m_noteAnnotationAction);
    editMenu->addAction(m_editNoteAnnotationAction);
    editMenu->addAction(m_editTextAction);
    editMenu->addAction(m_changeAnnotationColorAction);
    editMenu->addAction(m_redactSelectionAction);
    editMenu->addSeparator();
    editMenu->addAction(m_runOcrCurrentPageAction);
    editMenu->addAction(m_runOcrSelectionAction);
    editMenu->addSeparator();
    editMenu->addAction(m_findAction);
    editMenu->addAction(m_findNextAction);
    editMenu->addAction(m_findPreviousAction);
    editMenu->addAction(m_clearSearchAction);

    QMenu *toolsMenu = menuBar()->addMenu(QStringLiteral("Werkzeuge"));
    toolsMenu->addAction(m_highlightSelectionAction);
    toolsMenu->addAction(m_replaceSelectedTextAction);
    toolsMenu->addAction(m_addTextAction);
    toolsMenu->addAction(m_editTextAction);
    toolsMenu->addAction(m_rectangleAnnotationAction);
    toolsMenu->addAction(m_noteAnnotationAction);
    toolsMenu->addAction(m_editNoteAnnotationAction);
    toolsMenu->addAction(m_insertSignatureAction);
    toolsMenu->addAction(m_changeAnnotationColorAction);
    toolsMenu->addAction(m_editFormFieldTextStyleAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_redactSelectionAction);
    toolsMenu->addAction(m_runOcrCurrentPageAction);
    toolsMenu->addAction(m_runOcrSelectionAction);

    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("Ansicht"));
    viewMenu->addAction(m_zoomInAction);
    viewMenu->addAction(m_zoomOutAction);
    viewMenu->addAction(m_resetZoomAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_toggleDarkModeAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_navigationDock->toggleViewAction());
    viewMenu->addAction(m_inspectorDock->toggleViewAction());
    viewMenu->addAction(m_mainToolBar->toggleViewAction());
    viewMenu->addAction(m_toolsToolBar->toggleViewAction());

    QMenu *navigateMenu = menuBar()->addMenu(QStringLiteral("Navigation"));
    navigateMenu->addAction(m_previousPageAction);
    navigateMenu->addAction(m_nextPageAction);
    navigateMenu->addSeparator();
    navigateMenu->addAction(m_movePageLeftAction);
    navigateMenu->addAction(m_movePageRightAction);
    navigateMenu->addAction(m_deletePageAction);

    QMenu *windowMenu = menuBar()->addMenu(QStringLiteral("Fenster"));
    windowMenu->addAction(m_navigationDock->toggleViewAction());
    windowMenu->addAction(m_inspectorDock->toggleViewAction());
    windowMenu->addSeparator();
    windowMenu->addAction(m_mainToolBar->toggleViewAction());
    windowMenu->addAction(m_toolsToolBar->toggleViewAction());
}

void MainWindow::createToolbar()
{
    m_mainToolBar = addToolBar(QStringLiteral("Hauptwerkzeuge"));
    m_mainToolBar->setObjectName(QStringLiteral("MainToolbar"));
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_mainToolBar->setIconSize(QSize(18, 18));
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_saveAction);
    m_mainToolBar->addAction(m_printAction);
    m_mainToolBar->addAction(m_showMetadataAction);
    m_mainToolBar->addWidget(createToolbarSpacer(8));
    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);
    m_mainToolBar->addWidget(createToolbarSpacer(8));
    m_mainToolBar->addAction(m_previousPageAction);
    m_pageJumpSpinBox = new QSpinBox(this);
    m_pageJumpSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_pageJumpSpinBox->setAlignment(Qt::AlignCenter);
    m_pageJumpSpinBox->setFixedWidth(64);
    m_pageJumpSpinBox->setSpecialValueText(QStringLiteral("-"));
    m_pageJumpSpinBox->setToolTip(QStringLiteral("Zur Seite springen"));
    connect(m_pageJumpSpinBox, &QSpinBox::valueChanged, this, &MainWindow::jumpToPage);
    m_mainToolBar->addWidget(m_pageJumpSpinBox);
    m_pageCountHintLabel = new QLabel(QStringLiteral("Keine Seiten"), this);
    m_pageCountHintLabel->setMinimumWidth(58);
    m_mainToolBar->addWidget(m_pageCountHintLabel);
    m_mainToolBar->addAction(m_nextPageAction);
    m_mainToolBar->addWidget(createToolbarSpacer(8));
    m_mainToolBar->addAction(m_zoomOutAction);
    m_mainToolBar->addAction(m_zoomInAction);
    m_mainToolBar->addAction(m_resetZoomAction);
    m_mainToolBar->addWidget(createToolbarSpacer(10));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Im Dokument suchen"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(210);
    m_searchEdit->setMaximumWidth(250);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::triggerSearch);
    m_mainToolBar->addWidget(m_searchEdit);
    m_searchButton = new QPushButton(QStringLiteral("Suchen"), this);
    m_searchButton->setAutoDefault(false);
    m_searchButton->setIcon(loadToolbarIcon(QStringLiteral("search")));
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::triggerSearch);
    m_mainToolBar->addWidget(m_searchButton);
    m_mainToolBar->addAction(m_findNextAction);
    m_mainToolBar->addAction(m_findPreviousAction);
    m_mainToolBar->addAction(m_clearSearchAction);
    m_mainToolBar->addWidget(createToolbarSpacer(8));
    m_mainToolBar->addAction(m_toggleDarkModeAction);

    addToolBarBreak(Qt::TopToolBarArea);

    m_toolsToolBar = addToolBar(QStringLiteral("Werkzeuge"));
    m_toolsToolBar->setObjectName(QStringLiteral("ToolsToolbar"));
    m_toolsToolBar->setMovable(false);
    m_toolsToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolsToolBar->setIconSize(QSize(16, 16));
    m_toolsToolBar->addAction(m_highlightSelectionAction);
    m_toolsToolBar->addAction(m_replaceSelectedTextAction);
    m_toolsToolBar->addAction(m_addTextAction);
    m_toolsToolBar->addAction(m_editTextAction);
    m_toolsToolBar->addAction(m_rectangleAnnotationAction);
    m_toolsToolBar->addAction(m_noteAnnotationAction);
    m_toolsToolBar->addAction(m_insertSignatureAction);
    m_toolsToolBar->addAction(m_changeAnnotationColorAction);
    m_toolsToolBar->addAction(m_editFormFieldTextStyleAction);
    m_toolsToolBar->addAction(m_redactSelectionAction);
    m_toolsToolBar->addAction(m_deleteOverlayAction);
    m_toolsToolBar->addWidget(createToolbarSpacer(8));
    m_toolsToolBar->addAction(m_runOcrCurrentPageAction);
    m_toolsToolBar->addAction(m_runOcrSelectionAction);
    m_toolsToolBar->addWidget(createToolbarSpacer(8));
    m_toolsToolBar->addAction(m_mergePdfsAction);
    m_toolsToolBar->addAction(m_splitPdfAction);
    m_toolsToolBar->addAction(m_showMetadataAction);
    m_toolsToolBar->addWidget(createToolbarSpacer(8));
    m_toolsToolBar->addAction(m_exportEditedPdfAction);
    m_toolsToolBar->addAction(m_exportRedactedPdfAction);
    m_toolsToolBar->addAction(m_exportEncryptedPdfAction);
    m_toolsToolBar->addAction(m_exportSignedPdfAction);
}

bool MainWindow::promptForCryptographicSignature(
    QString &certNickname,
    QString &password,
    QString &reason,
    QString &location) const
{
    QDialog dialog(const_cast<MainWindow *>(this));
    dialog.setWindowTitle(QStringLiteral("PDF kryptografisch signieren"));

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(
        QStringLiteral("Bitte den Zertifikat-Nickname des aktiven Signatur-Backends angeben."),
        &dialog));

    auto *formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    auto *nicknameEdit = new QLineEdit(&dialog);
    auto *passwordEdit = new QLineEdit(&dialog);
    auto *reasonEdit = new QLineEdit(&dialog);
    auto *locationEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow(QStringLiteral("Zertifikat:"), nicknameEdit);
    formLayout->addRow(QStringLiteral("Passphrase:"), passwordEdit);
    formLayout->addRow(QStringLiteral("Grund:"), reasonEdit);
    formLayout->addRow(QStringLiteral("Ort:"), locationEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    certNickname = nicknameEdit->text();
    password = passwordEdit->text();
    reason = reasonEdit->text();
    location = locationEdit->text();
    return true;
}

bool MainWindow::signPdfWithPoppler(
    const QString &inputFile,
    const QString &outputFile,
    int pageIndex,
    const QRectF &pageRect,
    const QByteArray &signatureImageData,
    const QString &certNickname,
    const QString &password,
    const QString &reason,
    const QString &location)
{
    QTemporaryFile signatureImageFile(QDir::tempPath() + QStringLiteral("/pdftool-signature-XXXXXX.png"));
    signatureImageFile.setAutoRemove(true);
    if (!signatureImageFile.open()) {
        showError(QStringLiteral("Temporäre Signaturdatei konnte nicht angelegt werden."));
        return false;
    }
    if (signatureImageFile.write(signatureImageData) != signatureImageData.size()) {
        showError(QStringLiteral("Temporäres Signaturbild konnte nicht geschrieben werden."));
        return false;
    }
    signatureImageFile.flush();

    std::unique_ptr<Poppler::Document> document(
        Poppler::Document::load(
            inputFile,
            m_documentController->currentOwnerPassword(),
            m_documentController->currentUserPassword()));
    if (!document) {
        showError(QStringLiteral("Temporäres PDF konnte für die Signatur nicht geladen werden."));
        return false;
    }

    std::unique_ptr<Poppler::PDFConverter> converter = document->pdfConverter();
    if (!converter) {
        showError(QStringLiteral("Poppler-PDF-Konverter ist nicht verfügbar."));
        return false;
    }

    converter->setOutputFileName(outputFile);
    converter->setPDFOptions(Poppler::PDFConverter::WithChanges);

    Poppler::PDFConverter::NewSignatureData signatureData;
    signatureData.setCertNickname(certNickname);
    signatureData.setPassword(password);
    signatureData.setPage(pageIndex);
    signatureData.setBoundingRectangle(pageRect);
    signatureData.setSignatureText(certNickname);
    signatureData.setReason(reason);
    signatureData.setLocation(location);
    signatureData.setImagePath(signatureImageFile.fileName());
    signatureData.setDocumentOwnerPassword(m_documentController->currentOwnerPassword());
    signatureData.setDocumentUserPassword(m_documentController->currentUserPassword());

    if (!converter->sign(signatureData)) {
        showError(QStringLiteral("PDF konnte nicht kryptografisch signiert werden."));
        return false;
    }

    return true;
}

void MainWindow::createCentralLayout()
{
    m_pdfView = new PdfView(this);
    setCentralWidget(m_pdfView);

    m_navigationTabs = new QTabWidget(this);
    m_navigationTabs->setDocumentMode(true);

    m_pageListView = new QListView(this);
    m_pageListView->setViewMode(QListView::IconMode);
    m_pageListView->setFlow(QListView::TopToBottom);
    m_pageListView->setMovement(QListView::Static);
    m_pageListView->setResizeMode(QListView::Adjust);
    m_pageListView->setWrapping(false);
    m_pageListView->setSpacing(14);
    m_pageListView->setGridSize(QSize(182, 242));
    m_pageListView->setUniformItemSizes(false);
    m_pageListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageListView->setIconSize(QSize(140, 180));
    m_pageListView->setWordWrap(true);
    m_pageListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pageThumbnailModel = new PageThumbnailListModel(m_documentController.get(), m_pageListView);
    m_pageListView->setModel(m_pageThumbnailModel);
    connect(m_pageListView, &QListView::clicked, this, &MainWindow::navigateToSidebarPage);
    connect(m_pageListView, &QListView::activated, this, &MainWindow::navigateToSidebarPage);
    m_navigationTabs->addTab(m_pageListView, QStringLiteral("Seiten"));

    m_outlineTreeWidget = new QTreeWidget(this);
    m_outlineTreeWidget->setHeaderHidden(true);
    m_outlineTreeWidget->setAlternatingRowColors(true);
    connect(m_outlineTreeWidget, &QTreeWidget::itemActivated, this, &MainWindow::navigateToOutlineItem);
    connect(m_outlineTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::navigateToOutlineItem);
    m_navigationTabs->addTab(m_outlineTreeWidget, QStringLiteral("Inhalt"));

    m_navigationDock = new QDockWidget(QStringLiteral("Navigation"), this);
    m_navigationDock->setObjectName(QStringLiteral("NavigationDock"));
    m_navigationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_navigationDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_navigationDock->setMinimumWidth(250);
    m_navigationDock->setWidget(m_navigationTabs);
    addDockWidget(Qt::LeftDockWidgetArea, m_navigationDock);
}

void MainWindow::createInspectorDock()
{
    m_inspectorStack = new QStackedWidget(this);

    m_inspectorEmptyPage = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_inspectorEmptyPage);
    auto *emptyLabel = new QLabel(QStringLiteral("Kein Objekt ausgewählt.\n\nWähle eine Textnotiz, ein Textfeld oder eine Unterschrift aus."));
    emptyLabel->setWordWrap(true);
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();
    m_inspectorStack->addWidget(m_inspectorEmptyPage);

    m_inspectorTextPage = new QWidget(this);
    auto *textLayout = new QVBoxLayout(m_inspectorTextPage);
    auto *textForm = new QFormLayout();
    m_inspectorTextEdit = new QTextEdit(m_inspectorTextPage);
    m_inspectorFontFamilyBox = new QComboBox(m_inspectorTextPage);
    m_inspectorFontFamilyBox->addItems(supportedPdfFontFamilies());
    m_inspectorFontSizeSpin = new QSpinBox(m_inspectorTextPage);
    m_inspectorFontSizeSpin->setRange(6, 96);
    m_inspectorTextColorButton = new QPushButton(m_inspectorTextPage);
    m_inspectorBackgroundColorButton = new QPushButton(m_inspectorTextPage);
    textForm->addRow(QStringLiteral("Text:"), m_inspectorTextEdit);
    textForm->addRow(QStringLiteral("Schriftart:"), m_inspectorFontFamilyBox);
    textForm->addRow(QStringLiteral("Schriftgröße:"), m_inspectorFontSizeSpin);
    textForm->addRow(QStringLiteral("Schriftfarbe:"), m_inspectorTextColorButton);
    textForm->addRow(QStringLiteral("Hintergrund:"), m_inspectorBackgroundColorButton);
    textLayout->addLayout(textForm);
    auto *textApplyButton = new QPushButton(QStringLiteral("Textfeld anwenden"), m_inspectorTextPage);
    textLayout->addWidget(textApplyButton);
    textLayout->addStretch();
    m_inspectorStack->addWidget(m_inspectorTextPage);

    m_inspectorNotePage = new QWidget(this);
    auto *noteLayout = new QVBoxLayout(m_inspectorNotePage);
    auto *noteForm = new QFormLayout();
    m_inspectorNoteEdit = new QTextEdit(m_inspectorNotePage);
    m_inspectorNoteColorButton = new QPushButton(m_inspectorNotePage);
    noteForm->addRow(QStringLiteral("Kommentar:"), m_inspectorNoteEdit);
    noteForm->addRow(QStringLiteral("Farbe:"), m_inspectorNoteColorButton);
    noteLayout->addLayout(noteForm);
    auto *noteApplyButton = new QPushButton(QStringLiteral("Textnotiz anwenden"), m_inspectorNotePage);
    noteLayout->addWidget(noteApplyButton);
    noteLayout->addStretch();
    m_inspectorStack->addWidget(m_inspectorNotePage);

    m_inspectorSignaturePage = new QWidget(this);
    auto *signatureLayout = new QVBoxLayout(m_inspectorSignaturePage);
    auto *signatureLabel = new QLabel(QStringLiteral(
        "Unterschrift ausgewählt.\n\nVerschieben per Drag.\nGröße über den Ziehpunkt unten rechts ändern."));
    signatureLabel->setWordWrap(true);
    signatureLayout->addWidget(signatureLabel);
    signatureLayout->addStretch();
    m_inspectorStack->addWidget(m_inspectorSignaturePage);

    connect(textApplyButton, &QPushButton::clicked, this, &MainWindow::applyInspectorChanges);
    connect(noteApplyButton, &QPushButton::clicked, this, &MainWindow::applyInspectorChanges);

    connect(m_inspectorTextColorButton, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
            m_inspectorTextColor.isValid() ? m_inspectorTextColor : QColor(Qt::black),
            this,
            QStringLiteral("Schriftfarbe wählen"),
            QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        m_inspectorTextColor = selected;
        updateColorButton(m_inspectorTextColorButton, selected);
    });
    connect(m_inspectorBackgroundColorButton, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
            m_inspectorBackgroundColor.isValid() ? m_inspectorBackgroundColor : QColor(255, 255, 255, 230),
            this,
            QStringLiteral("Hintergrundfarbe wählen"),
            QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        m_inspectorBackgroundColor = selected;
        updateColorButton(m_inspectorBackgroundColorButton, selected);
    });
    connect(m_inspectorNoteColorButton, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
            m_inspectorNoteColor.isValid() ? m_inspectorNoteColor : QColor(255, 193, 7, 210),
            this,
            QStringLiteral("Notizfarbe wählen"),
            QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) {
            return;
        }
        m_inspectorNoteColor = selected;
        updateColorButton(m_inspectorNoteColorButton, selected);
    });

    m_inspectorDock = new QDockWidget(QStringLiteral("Eigenschaften"), this);
    m_inspectorDock->setObjectName(QStringLiteral("InspectorDock"));
    m_inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_inspectorDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_inspectorDock->setMinimumWidth(280);
    m_inspectorDock->setWidget(m_inspectorStack);
    addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);
}

void MainWindow::refreshInspector()
{
    if (!m_inspectorStack) {
        return;
    }

    if (!m_documentController->hasSelectedOverlay()) {
        m_inspectorStack->setCurrentWidget(m_inspectorEmptyPage);
        return;
    }

    if (m_documentController->hasSelectedTextEditAnnotation()) {
        const QSignalBlocker textBlocker(m_inspectorTextEdit);
        const QSignalBlocker fontBlocker(m_inspectorFontFamilyBox);
        const QSignalBlocker sizeBlocker(m_inspectorFontSizeSpin);
        m_inspectorTextEdit->setPlainText(m_documentController->selectedTextEditText());
        const PdfTextStyle style = m_documentController->selectedTextEditStyle();
        const QString fontFamily = normalizedPdfFontFamily(style.fontFamily);
        const int fontIndex = m_inspectorFontFamilyBox->findText(fontFamily);
        m_inspectorFontFamilyBox->setCurrentIndex(fontIndex >= 0 ? fontIndex : 0);
        m_inspectorFontSizeSpin->setValue(qRound(style.fontSize > 0.0 ? style.fontSize : 12.0));
        m_inspectorTextColor = style.textColor.isValid() ? style.textColor : QColor(Qt::black);
        m_inspectorBackgroundColor = m_documentController->selectedTextEditBackgroundColor().isValid()
            ? m_documentController->selectedTextEditBackgroundColor()
            : QColor(255, 255, 255, 230);
        updateColorButton(m_inspectorTextColorButton, m_inspectorTextColor);
        updateColorButton(m_inspectorBackgroundColorButton, m_inspectorBackgroundColor);
        m_inspectorStack->setCurrentWidget(m_inspectorTextPage);
        return;
    }

    if (m_documentController->hasSelectedNoteAnnotation()) {
        const QSignalBlocker noteBlocker(m_inspectorNoteEdit);
        m_inspectorNoteEdit->setPlainText(m_documentController->selectedNoteText());
        m_inspectorNoteColor = m_documentController->selectedAnnotationColor().isValid()
            ? m_documentController->selectedAnnotationColor()
            : QColor(255, 193, 7, 210);
        updateColorButton(m_inspectorNoteColorButton, m_inspectorNoteColor);
        m_inspectorStack->setCurrentWidget(m_inspectorNotePage);
        return;
    }

    if (m_documentController->hasSelectedSignatureAnnotation()) {
        m_inspectorStack->setCurrentWidget(m_inspectorSignaturePage);
        return;
    }

    m_inspectorStack->setCurrentWidget(m_inspectorEmptyPage);
}

void MainWindow::applyInspectorChanges()
{
    if (m_documentController->hasSelectedTextEditAnnotation()) {
        PdfTextStyle style;
        style.fontFamily = normalizedPdfFontFamily(m_inspectorFontFamilyBox->currentText());
        style.fontSize = m_inspectorFontSizeSpin->value();
        style.textColor = m_inspectorTextColor.isValid() ? m_inspectorTextColor : QColor(Qt::black);
        m_documentController->updateSelectedTextEdit(
            m_inspectorTextEdit->toPlainText(),
            style,
            m_inspectorBackgroundColor.isValid() ? m_inspectorBackgroundColor : QColor(255, 255, 255, 230));
        return;
    }

    if (m_documentController->hasSelectedNoteAnnotation()) {
        m_documentController->updateSelectedNoteText(m_inspectorNoteEdit->toPlainText());
        m_documentController->setSelectedAnnotationColor(
            m_inspectorNoteColor.isValid() ? m_inspectorNoteColor : QColor(255, 193, 7, 210));
    }
}

void MainWindow::createStatusBar()
{
    m_documentStatusLabel = createStatusPill(QStringLiteral("Kein Dokument"), QStringLiteral("statusPillAccent"));
    m_modeStatusLabel = createStatusPill(QStringLiteral("Bereit"), QStringLiteral("statusPill"));
    m_pageStatusLabel = createStatusPill(QStringLiteral("Seite: -/-"), QStringLiteral("statusPill"));
    m_zoomLabel = createStatusPill(QStringLiteral("Zoom: 100%"), QStringLiteral("statusPill"));
    m_searchStatusLabel = createStatusPill(QStringLiteral("Keine aktive Suche."), QStringLiteral("statusPillAccent"));

    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(m_documentStatusLabel);
    statusBar()->addWidget(m_modeStatusLabel);
    statusBar()->addPermanentWidget(m_pageStatusLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(m_searchStatusLabel, 1);
}

void MainWindow::refreshNavigationPanels()
{
    if (m_pageThumbnailModel) {
        m_pageThumbnailModel->refresh();
    }
    syncThumbnailSelection(m_documentController->currentPageIndex());

    m_outlineTreeWidget->clear();
    populateOutlineTree(m_documentController->outlineEntries());
}

void MainWindow::syncThumbnailSelection(int pageIndex)
{
    if (!m_pageListView || !m_pageThumbnailModel || !m_pageListView->selectionModel()) {
        return;
    }

    const QSignalBlocker blocker(m_pageListView->selectionModel());

    if (pageIndex < 0 || pageIndex >= m_pageThumbnailModel->rowCount()) {
        m_pageListView->clearSelection();
        return;
    }

    const QModelIndex modelIndex = m_pageThumbnailModel->index(pageIndex, 0);
    if (!modelIndex.isValid()) {
        return;
    }

    m_pageListView->selectionModel()->setCurrentIndex(
        modelIndex,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_pageListView->scrollTo(modelIndex, QAbstractItemView::PositionAtCenter);
}

void MainWindow::populateOutlineTree(const QVector<PdfOutlineEntry> &entries, QTreeWidgetItem *parentItem)
{
    for (const PdfOutlineEntry &entry : entries) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, entry.title.isEmpty() ? QStringLiteral("(ohne Titel)") : entry.title);
        item->setData(0, Qt::UserRole, entry.pageIndex);

        if (parentItem) {
            parentItem->addChild(item);
        } else {
            m_outlineTreeWidget->addTopLevelItem(item);
        }

        if (!entry.children.isEmpty()) {
            populateOutlineTree(entry.children, item);
        }

        item->setExpanded(entry.initiallyOpen);
    }
}
