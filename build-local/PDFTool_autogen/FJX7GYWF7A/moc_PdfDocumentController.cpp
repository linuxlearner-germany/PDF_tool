/****************************************************************************
** Meta object code from reading C++ file 'PdfDocumentController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/document/PdfDocumentController.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PdfDocumentController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN21PdfDocumentControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN21PdfDocumentControllerE = QtMocHelpers::stringData(
    "PdfDocumentController",
    "documentStateChanged",
    "",
    "hasDocument",
    "pageImageChanged",
    "image",
    "zoomChanged",
    "factor",
    "currentPageChanged",
    "pageIndex",
    "pageCount",
    "pageLabel",
    "selectionHighlightChanged",
    "QList<QRectF>",
    "imageRects",
    "searchHighlightChanged",
    "allImageRects",
    "currentImageRects",
    "annotationOverlaysChanged",
    "QList<PdfAnnotationOverlay>",
    "overlays",
    "formFieldOverlaysChanged",
    "QList<PdfFormFieldOverlay>",
    "redactionOverlaysChanged",
    "QList<PdfRedactionOverlay>",
    "textSelectionChanged",
    "hasSelection",
    "overlaySelectionChanged",
    "searchStateChanged",
    "hasResults",
    "currentHit",
    "totalHits",
    "statusText",
    "errorOccurred",
    "message",
    "statusMessageChanged",
    "busyStateChanged",
    "busy",
    "ocrFinished",
    "title",
    "text",
    "zoomIn",
    "zoomOut",
    "resetZoom",
    "setZoomFactor",
    "goToPage",
    "nextPage",
    "previousPage",
    "updateTextSelection",
    "imageSelectionRect",
    "clearTextSelection",
    "copySelectedText",
    "addHighlightAnnotationFromSelection",
    "addRectangleAnnotationFromSelection",
    "addNoteAnnotationAt",
    "imagePoint",
    "noteText",
    "addFreeTextAt",
    "replaceSelectedText",
    "addSignatureFromImageAt",
    "signatureImage",
    "addSignatureFromImageToSelection",
    "selectOverlayAt",
    "deleteSelectedOverlay",
    "setSelectedAnnotationColor",
    "color",
    "updateSelectedNoteText",
    "updateSelectedTextEdit",
    "setSearchQuery",
    "query",
    "findNext",
    "findPrevious",
    "clearSearch",
    "setFormFieldText",
    "fieldId",
    "setFormFieldChecked",
    "checked",
    "addRedactionFromSelection",
    "runOcrOnCurrentPage",
    "runOcrOnSelection"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN21PdfDocumentControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      47,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  296,    2, 0x06,    1 /* Public */,
       4,    1,  299,    2, 0x06,    3 /* Public */,
       6,    1,  302,    2, 0x06,    5 /* Public */,
       8,    3,  305,    2, 0x06,    7 /* Public */,
      12,    1,  312,    2, 0x06,   11 /* Public */,
      15,    2,  315,    2, 0x06,   13 /* Public */,
      18,    1,  320,    2, 0x06,   16 /* Public */,
      21,    1,  323,    2, 0x06,   18 /* Public */,
      23,    1,  326,    2, 0x06,   20 /* Public */,
      25,    1,  329,    2, 0x06,   22 /* Public */,
      27,    1,  332,    2, 0x06,   24 /* Public */,
      28,    4,  335,    2, 0x06,   26 /* Public */,
      33,    1,  344,    2, 0x06,   31 /* Public */,
      35,    1,  347,    2, 0x06,   33 /* Public */,
      36,    2,  350,    2, 0x06,   35 /* Public */,
      38,    2,  355,    2, 0x06,   38 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      41,    0,  360,    2, 0x0a,   41 /* Public */,
      42,    0,  361,    2, 0x0a,   42 /* Public */,
      43,    0,  362,    2, 0x0a,   43 /* Public */,
      44,    1,  363,    2, 0x0a,   44 /* Public */,
      45,    1,  366,    2, 0x0a,   46 /* Public */,
      46,    0,  369,    2, 0x0a,   48 /* Public */,
      47,    0,  370,    2, 0x0a,   49 /* Public */,
      48,    1,  371,    2, 0x0a,   50 /* Public */,
      50,    0,  374,    2, 0x0a,   52 /* Public */,
      51,    0,  375,    2, 0x0a,   53 /* Public */,
      52,    0,  376,    2, 0x0a,   54 /* Public */,
      53,    0,  377,    2, 0x0a,   55 /* Public */,
      54,    2,  378,    2, 0x0a,   56 /* Public */,
      57,    2,  383,    2, 0x0a,   59 /* Public */,
      58,    1,  388,    2, 0x0a,   62 /* Public */,
      59,    2,  391,    2, 0x0a,   64 /* Public */,
      61,    1,  396,    2, 0x0a,   67 /* Public */,
      62,    1,  399,    2, 0x0a,   69 /* Public */,
      63,    0,  402,    2, 0x0a,   71 /* Public */,
      64,    1,  403,    2, 0x0a,   72 /* Public */,
      66,    1,  406,    2, 0x0a,   74 /* Public */,
      67,    1,  409,    2, 0x0a,   76 /* Public */,
      68,    1,  412,    2, 0x0a,   78 /* Public */,
      70,    0,  415,    2, 0x0a,   80 /* Public */,
      71,    0,  416,    2, 0x0a,   81 /* Public */,
      72,    0,  417,    2, 0x0a,   82 /* Public */,
      73,    2,  418,    2, 0x0a,   83 /* Public */,
      75,    2,  423,    2, 0x0a,   86 /* Public */,
      77,    0,  428,    2, 0x0a,   89 /* Public */,
      78,    0,  429,    2, 0x0a,   90 /* Public */,
      79,    0,  430,    2, 0x0a,   91 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QImage,    5,
    QMetaType::Void, QMetaType::Double,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    9,   10,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 13, 0x80000000 | 13,   16,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, 0x80000000 | 22,   20,
    QMetaType::Void, 0x80000000 | 24,   20,
    QMetaType::Void, QMetaType::Bool,   26,
    QMetaType::Void, QMetaType::Bool,   26,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int, QMetaType::Int, QMetaType::QString,   29,   30,   31,   32,
    QMetaType::Void, QMetaType::QString,   34,
    QMetaType::Void, QMetaType::QString,   34,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   37,   34,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   39,   40,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QRectF,   49,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QString,   55,   56,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QString,   55,   40,
    QMetaType::Void, QMetaType::QString,   40,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QImage,   55,   60,
    QMetaType::Void, QMetaType::QImage,   60,
    QMetaType::Void, QMetaType::QPointF,   55,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QColor,   65,
    QMetaType::Void, QMetaType::QString,   40,
    QMetaType::Void, QMetaType::QString,   40,
    QMetaType::Void, QMetaType::QString,   69,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   74,   40,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   74,   76,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PdfDocumentController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN21PdfDocumentControllerE.offsetsAndSizes,
    qt_meta_data_ZN21PdfDocumentControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN21PdfDocumentControllerE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PdfDocumentController, std::true_type>,
        // method 'documentStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'pageImageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'zoomChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'currentPageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectionHighlightChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        // method 'searchHighlightChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        // method 'annotationOverlaysChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfAnnotationOverlay> &, std::false_type>,
        // method 'formFieldOverlaysChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfFormFieldOverlay> &, std::false_type>,
        // method 'redactionOverlaysChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfRedactionOverlay> &, std::false_type>,
        // method 'textSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'overlaySelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'searchStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'statusMessageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'busyStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'ocrFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'zoomIn'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomOut'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetZoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setZoomFactor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'goToPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'nextPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'previousPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateTextSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QRectF &, std::false_type>,
        // method 'clearTextSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'copySelectedText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addHighlightAnnotationFromSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addRectangleAnnotationFromSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addNoteAnnotationAt'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addFreeTextAt'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'replaceSelectedText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addSignatureFromImageAt'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'addSignatureFromImageToSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'selectOverlayAt'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        // method 'deleteSelectedOverlay'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setSelectedAnnotationColor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>,
        // method 'updateSelectedNoteText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateSelectedTextEdit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setSearchQuery'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'findNext'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'findPrevious'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setFormFieldText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setFormFieldChecked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'addRedactionFromSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runOcrOnCurrentPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runOcrOnSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PdfDocumentController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PdfDocumentController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->documentStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->pageImageChanged((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 2: _t->zoomChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 3: _t->currentPageChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 4: _t->selectionHighlightChanged((*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[1]))); break;
        case 5: _t->searchHighlightChanged((*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[2]))); break;
        case 6: _t->annotationOverlaysChanged((*reinterpret_cast< std::add_pointer_t<QList<PdfAnnotationOverlay>>>(_a[1]))); break;
        case 7: _t->formFieldOverlaysChanged((*reinterpret_cast< std::add_pointer_t<QList<PdfFormFieldOverlay>>>(_a[1]))); break;
        case 8: _t->redactionOverlaysChanged((*reinterpret_cast< std::add_pointer_t<QList<PdfRedactionOverlay>>>(_a[1]))); break;
        case 9: _t->textSelectionChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->overlaySelectionChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->searchStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 12: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->statusMessageChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->busyStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: _t->ocrFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 16: _t->zoomIn(); break;
        case 17: _t->zoomOut(); break;
        case 18: _t->resetZoom(); break;
        case 19: _t->setZoomFactor((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 20: _t->goToPage((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 21: _t->nextPage(); break;
        case 22: _t->previousPage(); break;
        case 23: _t->updateTextSelection((*reinterpret_cast< std::add_pointer_t<QRectF>>(_a[1]))); break;
        case 24: _t->clearTextSelection(); break;
        case 25: _t->copySelectedText(); break;
        case 26: _t->addHighlightAnnotationFromSelection(); break;
        case 27: _t->addRectangleAnnotationFromSelection(); break;
        case 28: _t->addNoteAnnotationAt((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 29: _t->addFreeTextAt((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 30: _t->replaceSelectedText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 31: _t->addSignatureFromImageAt((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QImage>>(_a[2]))); break;
        case 32: _t->addSignatureFromImageToSelection((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 33: _t->selectOverlayAt((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 34: _t->deleteSelectedOverlay(); break;
        case 35: _t->setSelectedAnnotationColor((*reinterpret_cast< std::add_pointer_t<QColor>>(_a[1]))); break;
        case 36: _t->updateSelectedNoteText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 37: _t->updateSelectedTextEdit((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 38: _t->setSearchQuery((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 39: _t->findNext(); break;
        case 40: _t->findPrevious(); break;
        case 41: _t->clearSearch(); break;
        case 42: _t->setFormFieldText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 43: _t->setFormFieldChecked((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 44: _t->addRedactionFromSelection(); break;
        case 45: _t->runOcrOnCurrentPage(); break;
        case 46: _t->runOcrOnSelection(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QRectF> >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QRectF> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PdfDocumentController::*)(bool );
            if (_q_method_type _q_method = &PdfDocumentController::documentStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QImage & );
            if (_q_method_type _q_method = &PdfDocumentController::pageImageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(double );
            if (_q_method_type _q_method = &PdfDocumentController::zoomChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(int , int , const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::currentPageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QVector<QRectF> & );
            if (_q_method_type _q_method = &PdfDocumentController::selectionHighlightChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QVector<QRectF> & , const QVector<QRectF> & );
            if (_q_method_type _q_method = &PdfDocumentController::searchHighlightChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QVector<PdfAnnotationOverlay> & );
            if (_q_method_type _q_method = &PdfDocumentController::annotationOverlaysChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QVector<PdfFormFieldOverlay> & );
            if (_q_method_type _q_method = &PdfDocumentController::formFieldOverlaysChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QVector<PdfRedactionOverlay> & );
            if (_q_method_type _q_method = &PdfDocumentController::redactionOverlaysChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(bool );
            if (_q_method_type _q_method = &PdfDocumentController::textSelectionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(bool );
            if (_q_method_type _q_method = &PdfDocumentController::overlaySelectionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(bool , int , int , const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::searchStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::errorOccurred; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::statusMessageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(bool , const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::busyStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _q_method_type = void (PdfDocumentController::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &PdfDocumentController::ocrFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
    }
}

const QMetaObject *PdfDocumentController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PdfDocumentController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN21PdfDocumentControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PdfDocumentController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    }
    return _id;
}

// SIGNAL 0
void PdfDocumentController::documentStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PdfDocumentController::pageImageChanged(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PdfDocumentController::zoomChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PdfDocumentController::currentPageChanged(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void PdfDocumentController::selectionHighlightChanged(const QVector<QRectF> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PdfDocumentController::searchHighlightChanged(const QVector<QRectF> & _t1, const QVector<QRectF> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void PdfDocumentController::annotationOverlaysChanged(const QVector<PdfAnnotationOverlay> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void PdfDocumentController::formFieldOverlaysChanged(const QVector<PdfFormFieldOverlay> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void PdfDocumentController::redactionOverlaysChanged(const QVector<PdfRedactionOverlay> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void PdfDocumentController::textSelectionChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void PdfDocumentController::overlaySelectionChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void PdfDocumentController::searchStateChanged(bool _t1, int _t2, int _t3, const QString & _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void PdfDocumentController::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void PdfDocumentController::statusMessageChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void PdfDocumentController::busyStateChanged(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void PdfDocumentController::ocrFinished(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}
QT_WARNING_POP
