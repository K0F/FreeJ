/****************************************************************************
** Meta object code from reading C++ file 'QqWidget.h'
**
** Created: Thu Nov 4 20:50:48 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "QqWidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QqWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_FakeWindow[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_FakeWindow[] = {
    "FakeWindow\0"
};

const QMetaObject FakeWindow::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_FakeWindow,
      qt_meta_data_FakeWindow, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &FakeWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *FakeWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *FakeWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_FakeWindow))
        return static_cast<void*>(const_cast< FakeWindow*>(this));
    return QWidget::qt_metacast(_clname);
}

int FakeWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_QqTabWidget[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   13,   12,   12, 0x0a,
      34,   12,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QqTabWidget[] = {
    "QqTabWidget\0\0,\0moveLayer(int,int)\0"
    "closeTab(int)\0"
};

const QMetaObject QqTabWidget::staticMetaObject = {
    { &QTabWidget::staticMetaObject, qt_meta_stringdata_QqTabWidget,
      qt_meta_data_QqTabWidget, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QqTabWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QqTabWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QqTabWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QqTabWidget))
        return static_cast<void*>(const_cast< QqTabWidget*>(this));
    return QTabWidget::qt_metacast(_clname);
}

int QqTabWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: moveLayer((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: closeTab((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 2;
    }
    return _id;
}
static const uint qt_meta_data_QqWidget[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x0a,
      21,    9,    9,    9, 0x0a,
      36,    9,    9,    9, 0x0a,
      56,    9,    9,    9, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QqWidget[] = {
    "QqWidget\0\0slowDown()\0modTextLayer()\0"
    "changeFontSize(int)\0chgSize()\0"
};

const QMetaObject QqWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QqWidget,
      qt_meta_data_QqWidget, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &QqWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *QqWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QqWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QqWidget))
        return static_cast<void*>(const_cast< QqWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int QqWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: slowDown(); break;
        case 1: modTextLayer(); break;
        case 2: changeFontSize((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: chgSize(); break;
        default: ;
        }
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
