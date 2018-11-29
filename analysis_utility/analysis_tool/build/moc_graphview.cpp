/****************************************************************************
** Meta object code from reading C++ file 'graphview.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../src/profile/graphview.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'graphview.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GraphView_t {
    QByteArrayData data[8];
    char stringdata0[104];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GraphView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GraphView_t qt_meta_stringdata_GraphView = {
    {
QT_MOC_LITERAL(0, 0, 9), // "GraphView"
QT_MOC_LITERAL(1, 10, 11), // "zoomInEvent"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 12), // "zoomOutEvent"
QT_MOC_LITERAL(4, 36, 16), // "powerMaxDecEvent"
QT_MOC_LITERAL(5, 53, 16), // "powerMaxIncEvent"
QT_MOC_LITERAL(6, 70, 16), // "powerMinDecEvent"
QT_MOC_LITERAL(7, 87, 16) // "powerMinIncEvent"

    },
    "GraphView\0zoomInEvent\0\0zoomOutEvent\0"
    "powerMaxDecEvent\0powerMaxIncEvent\0"
    "powerMinDecEvent\0powerMinIncEvent"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GraphView[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x0a /* Public */,
       3,    0,   45,    2, 0x0a /* Public */,
       4,    0,   46,    2, 0x0a /* Public */,
       5,    0,   47,    2, 0x0a /* Public */,
       6,    0,   48,    2, 0x0a /* Public */,
       7,    0,   49,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void GraphView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        GraphView *_t = static_cast<GraphView *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->zoomInEvent(); break;
        case 1: _t->zoomOutEvent(); break;
        case 2: _t->powerMaxDecEvent(); break;
        case 3: _t->powerMaxIncEvent(); break;
        case 4: _t->powerMinDecEvent(); break;
        case 5: _t->powerMinIncEvent(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject GraphView::staticMetaObject = {
    { &QGraphicsView::staticMetaObject, qt_meta_stringdata_GraphView.data,
      qt_meta_data_GraphView,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *GraphView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GraphView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GraphView.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int GraphView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
