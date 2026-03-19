#pragma once

#include <memory>

#include <QMainWindow>
#include <QPoint>
#include <QPointF>

#include "document/PdfDocumentTypes.h"

class QAction;
class QLabel;
class QLineEdit;
class QListView;
class QDockWidget;
class QTextEdit;
class QTabWidget;
class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;
class PdfDocumentController;
class PdfView;
class QPdfOperations;
class PageThumbnailListModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openPdf();
    void triggerSearch();
    void showMetadataDialog();
    void mergePdfFiles();
    void splitCurrentPdf();
    void exportEncryptedPdf();
    void exportEditedPdf();
    void exportRedactedPdf();
    void printDocument();
    void insertSignature();
    void runOcrCurrentPage();
    void runOcrSelection();
    void showOcrResult(const QString &title, const QString &text);
    void requestNoteAnnotation();
    void editSelectedNoteAnnotation();
    void requestFreeTextAnnotation();
    void replaceSelectedText();
    void editSelectedTextAnnotation();
    void changeSelectedAnnotationColor();
    void setDarkMode(bool enabled);
    void updateWindowTitle();
    void updateZoomLabel(double factor);
    void updateCurrentPage(int pageIndex, int pageCount, const QString &pageLabel);
    void updateUiState(bool hasDocument);
    void updateSelectionDependentActions(bool hasSelection);
    void updateOverlaySelectionActions(bool hasSelection);
    void updateSearchState(bool hasResults, int currentHit, int totalHits, const QString &statusText);
    void navigateToSidebarPage(const QModelIndex &index);
    void navigateToOutlineItem(QTreeWidgetItem *item, int column);
    void focusSearchField();
    void showError(const QString &message);
    void showPdfViewContextMenu(const QPointF &imagePoint, const QPoint &globalPos);

private:
    enum class ThemeMode
    {
        Light,
        Dark
    };

    void createActions();
    void createMenus();
    void createToolbar();
    void createCentralLayout();
    void createStatusBar();
    void applyWindowStyle();
    void applyTheme(ThemeMode mode);
    ThemeMode loadThemeSetting() const;
    void saveThemeSetting(ThemeMode mode) const;
    QWidget *createToolbarSpacer(int width = 16) const;
    QLabel *createStatusPill(const QString &text, const QString &objectName) const;
    void refreshNavigationPanels();
    void populateOutlineTree(const QVector<PdfOutlineEntry> &entries, QTreeWidgetItem *parentItem = nullptr);
    void syncThumbnailSelection(int pageIndex);

    PdfView *m_pdfView {nullptr};
    QTabWidget *m_navigationTabs {nullptr};
    QListView *m_pageListView {nullptr};
    QTreeWidget *m_outlineTreeWidget {nullptr};
    QLineEdit *m_searchEdit {nullptr};
    QLabel *m_documentStatusLabel {nullptr};
    QLabel *m_modeStatusLabel {nullptr};
    QLabel *m_zoomLabel {nullptr};
    QLabel *m_pageStatusLabel {nullptr};
    QLabel *m_searchStatusLabel {nullptr};
    QDockWidget *m_navigationDock {nullptr};
    QToolBar *m_mainToolBar {nullptr};
    std::unique_ptr<PdfDocumentController> m_documentController;
    std::unique_ptr<QPdfOperations> m_pdfOperations;
    PageThumbnailListModel *m_pageThumbnailModel {nullptr};
    QPointF m_lastContextImagePoint;

    QAction *m_openAction {nullptr};
    QAction *m_mergePdfsAction {nullptr};
    QAction *m_splitPdfAction {nullptr};
    QAction *m_exportEncryptedPdfAction {nullptr};
    QAction *m_exportEditedPdfAction {nullptr};
    QAction *m_exportRedactedPdfAction {nullptr};
    QAction *m_printAction {nullptr};
    QAction *m_showMetadataAction {nullptr};
    QAction *m_exitAction {nullptr};
    QAction *m_zoomInAction {nullptr};
    QAction *m_zoomOutAction {nullptr};
    QAction *m_resetZoomAction {nullptr};
    QAction *m_previousPageAction {nullptr};
    QAction *m_nextPageAction {nullptr};
    QAction *m_copyAction {nullptr};
    QAction *m_deleteOverlayAction {nullptr};
    QAction *m_highlightSelectionAction {nullptr};
    QAction *m_rectangleAnnotationAction {nullptr};
    QAction *m_noteAnnotationAction {nullptr};
    QAction *m_editNoteAnnotationAction {nullptr};
    QAction *m_addTextAction {nullptr};
    QAction *m_replaceSelectedTextAction {nullptr};
    QAction *m_editTextAction {nullptr};
    QAction *m_changeAnnotationColorAction {nullptr};
    QAction *m_redactSelectionAction {nullptr};
    QAction *m_findAction {nullptr};
    QAction *m_findNextAction {nullptr};
    QAction *m_findPreviousAction {nullptr};
    QAction *m_clearSearchAction {nullptr};
    QAction *m_toggleDarkModeAction {nullptr};
    QAction *m_insertSignatureAction {nullptr};
    QAction *m_runOcrCurrentPageAction {nullptr};
    QAction *m_runOcrSelectionAction {nullptr};
    ThemeMode m_themeMode {ThemeMode::Light};
};
