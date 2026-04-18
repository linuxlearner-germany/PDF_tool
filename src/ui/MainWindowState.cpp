#include "ui/MainWindow.h"

#include <QFile>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

#include "document/PdfDocumentController.h"
#include "document/PdfDocumentTypes.h"
#include "operations/QPdfOperations.h"
#include "ui/MainWindowSupport.h"
#include "ui/PageThumbnailListModel.h"
#include "ui/PdfView.h"

using namespace MainWindowSupport;

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

void MainWindow::updateCapabilityHints()
{
    const QString qpdfError = m_pdfOperations->availabilityError();
    const QString ocrError = m_documentController->ocrAvailabilityError();

    const QString nativeExportTip = qpdfError.isEmpty()
        ? QStringLiteral("Bearbeitetes PDF mit nativen PDF-Aenderungen exportieren.")
        : QStringLiteral("Bearbeitetes PDF exportieren. %1").arg(qpdfError + QStringLiteral(" Fallback: gerastertes PDF."));
    m_exportEditedPdfAction->setToolTip(nativeExportTip);
    m_exportEditedPdfAction->setStatusTip(nativeExportTip);

    const QString redactedExportTip = qpdfError.isEmpty()
        ? QStringLiteral("Geschwaerztes PDF mit nativer Redaction exportieren.")
        : QStringLiteral("Geschwaerztes PDF exportieren. %1").arg(qpdfError + QStringLiteral(" Fallback: gerastertes PDF."));
    m_exportRedactedPdfAction->setToolTip(redactedExportTip);
    m_exportRedactedPdfAction->setStatusTip(redactedExportTip);

    const QString qpdfDependentTip = qpdfError.isEmpty()
        ? QStringLiteral("Native PDF-Operation mit qpdf.")
        : qpdfError;
    m_mergePdfsAction->setToolTip(qpdfDependentTip);
    m_splitPdfAction->setToolTip(qpdfDependentTip);
    m_exportEncryptedPdfAction->setToolTip(qpdfDependentTip);
    m_movePageLeftAction->setToolTip(qpdfDependentTip);
    m_movePageRightAction->setToolTip(qpdfDependentTip);
    m_deletePageAction->setToolTip(qpdfDependentTip);

    const QString ocrTip = ocrError.isEmpty()
        ? QStringLiteral("OCR mit lokalem Tesseract ausfuehren.")
        : ocrError;
    m_runOcrCurrentPageAction->setToolTip(ocrTip);
    m_runOcrSelectionAction->setToolTip(ocrTip);

    if (m_capabilityStatusLabel) {
        const QString qpdfStatus = qpdfError.isEmpty() ? QStringLiteral("qpdf bereit") : QStringLiteral("qpdf fehlt");
        const QString ocrStatus = ocrError.isEmpty() ? QStringLiteral("OCR bereit") : QStringLiteral("OCR fehlt");
        m_capabilityStatusLabel->setText(QStringLiteral("%1 | %2").arg(qpdfStatus, ocrStatus));
        m_capabilityStatusLabel->setToolTip(
            QStringLiteral("%1\n%2")
                .arg(qpdfError.isEmpty() ? QStringLiteral("qpdf aktiv.") : qpdfError)
                .arg(ocrError.isEmpty() ? QStringLiteral("Tesseract aktiv.") : ocrError));
    }
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
    const QString backupPath = documentPath + QStringLiteral(".pageops.bak.pdf");

    QFile::remove(tempPath);
    QFile::remove(backupPath);
    if (!m_pdfOperations->reorderPages(documentPath, tempPath, newOrder, userPassword)) {
        showError(m_pdfOperations->lastError());
        return false;
    }

    QString replaceError;
    if (!replaceDocumentWithBackup(tempPath, documentPath, backupPath, replaceError)) {
        QFile::remove(tempPath);
        showError(replaceError);
        return false;
    }

    if (!m_documentController->remapPageOrder(newOrder)) {
        QString restoreError;
        restoreDocumentFromBackup(documentPath, backupPath, restoreError);
        m_documentController->openDocument(documentPath, ownerPassword, userPassword);
        showError(QStringLiteral("Begleitdaten konnten nicht auf die neue Seitenreihenfolge angepasst werden.%1")
                      .arg(restoreError.isEmpty() ? QString() : QStringLiteral("\nRollback: %1").arg(restoreError)));
        return false;
    }

    if (!m_documentController->openDocument(documentPath, ownerPassword, userPassword)) {
        const QString reopenError = m_documentController->lastError();
        QString restoreError;
        restoreDocumentFromBackup(documentPath, backupPath, restoreError);
        m_documentController->openDocument(documentPath, ownerPassword, userPassword);
        showError(QStringLiteral("Die aktualisierte PDF konnte nicht erneut geladen werden: %1%2")
                      .arg(reopenError)
                      .arg(restoreError.isEmpty() ? QString() : QStringLiteral("\nRollback: %1").arg(restoreError)));
        return false;
    }

    QFile::remove(backupPath);
    refreshNavigationPanels();
    updateWindowTitle();
    if (reopenedPageIndex >= 0 && reopenedPageIndex < m_documentController->pageCount()) {
        m_documentController->goToPage(reopenedPageIndex);
    }
    statusBar()->showMessage(successMessage, 4000);
    return true;
}

bool MainWindow::replaceDocumentWithBackup(
    const QString &stagedPath,
    const QString &documentPath,
    const QString &backupPath,
    QString &errorMessage) const
{
    errorMessage.clear();

    if (QFile::exists(documentPath) && !QFile::rename(documentPath, backupPath)) {
        errorMessage = QStringLiteral("Originaldatei konnte nicht gesichert werden.");
        return false;
    }

    if (!QFile::rename(stagedPath, documentPath)) {
        if (QFile::exists(backupPath)) {
            QFile::rename(backupPath, documentPath);
        }
        errorMessage = QStringLiteral("Temporaere PDF-Datei konnte nicht uebernommen werden.");
        return false;
    }

    return true;
}

bool MainWindow::restoreDocumentFromBackup(
    const QString &documentPath,
    const QString &backupPath,
    QString &errorMessage) const
{
    errorMessage.clear();

    if (!QFile::exists(backupPath)) {
        return true;
    }

    QFile::remove(documentPath);
    if (!QFile::rename(backupPath, documentPath)) {
        errorMessage = QStringLiteral("Originaldatei konnte aus dem Backup nicht wiederhergestellt werden.");
        return false;
    }

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
