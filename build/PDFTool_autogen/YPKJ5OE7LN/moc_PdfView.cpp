/****************************************************************************
** Meta object code from reading C++ file 'PdfView.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/PdfView.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PdfView.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN7PdfViewE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN7PdfViewE = QtMocHelpers::stringData(
    "PdfView",
    "selectionRectChanged",
    "",
    "imageRect",
    "selectionCleared",
    "copyRequested",
    "deleteRequested",
    "nextPageRequested",
    "previousPageRequested",
    "zoomInRequested",
    "zoomOutRequested",
    "pointActivated",
    "imagePoint",
    "signatureMoveRequested",
    "imageDelta",
    "textEditResizeRequested",
    "contextMenuRequested",
    "globalPos",
    "formTextEdited",
    "fieldId",
    "text",
    "formCheckToggled",
    "checked",
    "setDarkMode",
    "enabled",
    "setPageImage",
    "image",
    "setSelectionHighlights",
    "QList<QRectF>",
    "imageRects",
    "setSearchHighlights",
    "currentImageRects",
    "setAnnotationOverlays",
    "QList<PdfAnnotationOverlay>",
    "overlays",
    "setFormFieldOverlays",
    "QList<PdfFormFieldOverlay>",
    "setRedactionOverlays",
    "QList<PdfRedactionOverlay>"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN7PdfViewE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  140,    2, 0x06,    1 /* Public */,
       4,    0,  143,    2, 0x06,    3 /* Public */,
       5,    0,  144,    2, 0x06,    4 /* Public */,
       6,    0,  145,    2, 0x06,    5 /* Public */,
       7,    0,  146,    2, 0x06,    6 /* Public */,
       8,    0,  147,    2, 0x06,    7 /* Public */,
       9,    0,  148,    2, 0x06,    8 /* Public */,
      10,    0,  149,    2, 0x06,    9 /* Public */,
      11,    1,  150,    2, 0x06,   10 /* Public */,
      13,    1,  153,    2, 0x06,   12 /* Public */,
      15,    1,  156,    2, 0x06,   14 /* Public */,
      16,    2,  159,    2, 0x06,   16 /* Public */,
      18,    2,  164,    2, 0x06,   19 /* Public */,
      21,    2,  169,    2, 0x06,   22 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      23,    1,  174,    2, 0x0a,   25 /* Public */,
      25,    1,  177,    2, 0x0a,   27 /* Public */,
      27,    1,  180,    2, 0x0a,   29 /* Public */,
      30,    2,  183,    2, 0x0a,   31 /* Public */,
      32,    1,  188,    2, 0x0a,   34 /* Public */,
      35,    1,  191,    2, 0x0a,   36 /* Public */,
      37,    1,  194,    2, 0x0a,   38 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QRectF,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF,   12,
    QMetaType::Void, QMetaType::QPointF,   14,
    QMetaType::Void, QMetaType::QPointF,   14,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPoint,   12,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   19,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   19,   22,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void, QMetaType::QImage,   26,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 28,   29,   31,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void, 0x80000000 | 36,   34,
    QMetaType::Void, 0x80000000 | 38,   34,

       0        // eod
};

Q_CONSTINIT const QMetaObject PdfView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_meta_stringdata_ZN7PdfViewE.offsetsAndSizes,
    qt_meta_data_ZN7PdfViewE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN7PdfViewE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PdfView, std::true_type>,
        // method 'selectionRectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QRectF &, std::false_type>,
        // method 'selectionCleared'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'copyRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deleteRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'nextPageRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'previousPageRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomInRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomOutRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pointActivated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        // method 'signatureMoveRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        // method 'textEditResizeRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        // method 'contextMenuRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'formTextEdited'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'formCheckToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setDarkMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setPageImage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'setSelectionHighlights'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        // method 'setSearchHighlights'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<QRectF> &, std::false_type>,
        // method 'setAnnotationOverlays'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfAnnotationOverlay> &, std::false_type>,
        // method 'setFormFieldOverlays'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfFormFieldOverlay> &, std::false_type>,
        // method 'setRedactionOverlays'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVector<PdfRedactionOverlay> &, std::false_type>
    >,
    nullptr
} };

void PdfView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PdfView *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->selectionRectChanged((*reinterpret_cast< std::add_pointer_t<QRectF>>(_a[1]))); break;
        case 1: _t->selectionCleared(); break;
        case 2: _t->copyRequested(); break;
        case 3: _t->deleteRequested(); break;
        case 4: _t->nextPageRequested(); break;
        case 5: _t->previousPageRequested(); break;
        case 6: _t->zoomInRequested(); break;
        case 7: _t->zoomOutRequested(); break;
        case 8: _t->pointActivated((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 9: _t->signatureMoveRequested((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 10: _t->textEditResizeRequested((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 11: _t->contextMenuRequested((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 12: _t->formTextEdited((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->formCheckToggled((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 14: _t->setDarkMode((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->setPageImage((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 16: _t->setSelectionHighlights((*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[1]))); break;
        case 17: _t->setSearchHighlights((*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<QRectF>>>(_a[2]))); break;
        case 18: _t->setAnnotationOverlays((*reinterpret_cast< std::add_pointer_t<QList<PdfAnnotationOverlay>>>(_a[1]))); break;
        case 19: _t->setFormFieldOverlays((*reinterpret_cast< std::add_pointer_t<QList<PdfFormFieldOverlay>>>(_a[1]))); break;
        case 20: _t->setRedactionOverlays((*reinterpret_cast< std::add_pointer_t<QList<PdfRedactionOverlay>>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 16:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QRectF> >(); break;
            }
            break;
        case 17:
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
            using _q_method_type = void (PdfView::*)(const QRectF & );
            if (_q_method_type _q_method = &PdfView::selectionRectChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::selectionCleared; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::copyRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::deleteRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::nextPageRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::previousPageRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::zoomInRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)();
            if (_q_method_type _q_method = &PdfView::zoomOutRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QPointF & );
            if (_q_method_type _q_method = &PdfView::pointActivated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QPointF & );
            if (_q_method_type _q_method = &PdfView::signatureMoveRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QPointF & );
            if (_q_method_type _q_method = &PdfView::textEditResizeRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QPointF & , const QPoint & );
            if (_q_method_type _q_method = &PdfView::contextMenuRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &PdfView::formTextEdited; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (PdfView::*)(const QString & , bool );
            if (_q_method_type _q_method = &PdfView::formCheckToggled; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
    }
}

const QMetaObject *PdfView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PdfView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN7PdfViewE.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int PdfView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void PdfView::selectionRectChanged(const QRectF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PdfView::selectionCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PdfView::copyRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PdfView::deleteRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PdfView::nextPageRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void PdfView::previousPageRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void PdfView::zoomInRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void PdfView::zoomOutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void PdfView::pointActivated(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void PdfView::signatureMoveRequested(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void PdfView::textEditResizeRequested(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void PdfView::contextMenuRequested(const QPointF & _t1, const QPoint & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void PdfView::formTextEdited(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void PdfView::formCheckToggled(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
QT_WARNING_POP
