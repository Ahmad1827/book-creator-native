/****************************************************************************
** Meta object code from reading C++ file 'layers_dock.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/layers_dock.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'layers_dock.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_LayersDock_t {
    uint offsetsAndSizes[34];
    char stringdata0[11];
    char stringdata1[23];
    char stringdata2[1];
    char stringdata3[5];
    char stringdata4[13];
    char stringdata5[18];
    char stringdata6[21];
    char stringdata7[8];
    char stringdata8[8];
    char stringdata9[26];
    char stringdata10[20];
    char stringdata11[23];
    char stringdata12[10];
    char stringdata13[8];
    char stringdata14[21];
    char stringdata15[21];
    char stringdata16[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_LayersDock_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_LayersDock_t qt_meta_stringdata_LayersDock = {
    {
        QT_MOC_LITERAL(0, 10),  // "LayersDock"
        QT_MOC_LITERAL(11, 22),  // "applyTemplateRequested"
        QT_MOC_LITERAL(34, 0),  // ""
        QT_MOC_LITERAL(35, 4),  // "side"
        QT_MOC_LITERAL(40, 12),  // "templateType"
        QT_MOC_LITERAL(53, 17),  // "addLayerRequested"
        QT_MOC_LITERAL(71, 20),  // "renameLayerRequested"
        QT_MOC_LITERAL(92, 7),  // "layerId"
        QT_MOC_LITERAL(100, 7),  // "newName"
        QT_MOC_LITERAL(108, 25),  // "toggleVisibilityRequested"
        QT_MOC_LITERAL(134, 19),  // "toggleLockRequested"
        QT_MOC_LITERAL(154, 22),  // "reorderLayersRequested"
        QT_MOC_LITERAL(177, 9),  // "fromIndex"
        QT_MOC_LITERAL(187, 7),  // "toIndex"
        QT_MOC_LITERAL(195, 20),  // "deleteLayerRequested"
        QT_MOC_LITERAL(216, 20),  // "selectLayerRequested"
        QT_MOC_LITERAL(237, 17)   // "activeSideChanged"
    },
    "LayersDock",
    "applyTemplateRequested",
    "",
    "side",
    "templateType",
    "addLayerRequested",
    "renameLayerRequested",
    "layerId",
    "newName",
    "toggleVisibilityRequested",
    "toggleLockRequested",
    "reorderLayersRequested",
    "fromIndex",
    "toIndex",
    "deleteLayerRequested",
    "selectLayerRequested",
    "activeSideChanged"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_LayersDock[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   68,    2, 0x06,    1 /* Public */,
       5,    0,   73,    2, 0x06,    4 /* Public */,
       6,    2,   74,    2, 0x06,    5 /* Public */,
       9,    1,   79,    2, 0x06,    8 /* Public */,
      10,    1,   82,    2, 0x06,   10 /* Public */,
      11,    2,   85,    2, 0x06,   12 /* Public */,
      14,    1,   90,    2, 0x06,   15 /* Public */,
      15,    1,   93,    2, 0x06,   17 /* Public */,
      16,    1,   96,    2, 0x06,   19 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   12,   13,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    3,

       0        // eod
};

Q_CONSTINIT const QMetaObject LayersDock::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_LayersDock.offsetsAndSizes,
    qt_meta_data_LayersDock,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_LayersDock_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LayersDock, std::true_type>,
        // method 'applyTemplateRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addLayerRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'renameLayerRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'toggleVisibilityRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'toggleLockRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'reorderLayersRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'deleteLayerRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectLayerRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'activeSideChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void LayersDock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LayersDock *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->applyTemplateRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->addLayerRequested(); break;
        case 2: _t->renameLayerRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->toggleVisibilityRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->toggleLockRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->reorderLayersRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->deleteLayerRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->selectLayerRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->activeSideChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LayersDock::*)(const QString & , const QString & );
            if (_t _q_method = &LayersDock::applyTemplateRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)();
            if (_t _q_method = &LayersDock::addLayerRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & , const QString & );
            if (_t _q_method = &LayersDock::renameLayerRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & );
            if (_t _q_method = &LayersDock::toggleVisibilityRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & );
            if (_t _q_method = &LayersDock::toggleLockRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(int , int );
            if (_t _q_method = &LayersDock::reorderLayersRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & );
            if (_t _q_method = &LayersDock::deleteLayerRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & );
            if (_t _q_method = &LayersDock::selectLayerRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (LayersDock::*)(const QString & );
            if (_t _q_method = &LayersDock::activeSideChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject *LayersDock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LayersDock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LayersDock.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int LayersDock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void LayersDock::applyTemplateRequested(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LayersDock::addLayerRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LayersDock::renameLayerRequested(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void LayersDock::toggleVisibilityRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LayersDock::toggleLockRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void LayersDock::reorderLayersRequested(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void LayersDock::deleteLayerRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void LayersDock::selectLayerRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void LayersDock::activeSideChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
