#include "ui/MainWindow.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QKeySequence>
#include <QSize>
#include <QSpinBox>
#include <QStyle>

#include "document/PdfDocumentController.h"
#include "ui/MainWindowSupport.h"

using namespace MainWindowSupport;

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
