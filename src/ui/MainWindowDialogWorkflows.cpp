#include "ui/MainWindow.h"

#include <QClipboard>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "document/PdfDocumentController.h"
#include "document/PdfDocumentTypes.h"
#include "ui/MainWindowSupport.h"

using namespace MainWindowSupport;

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
