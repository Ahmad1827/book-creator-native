/****************************************************************************
** Meta object code from reading C++ file 'bookshelf_view.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/bookshelf_view.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'bookshelf_view.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_BookshelfView_t {
    uint offsetsAndSizes[22];
    char stringdata0[14];
    char stringdata1[21];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[8];
    char stringdata5[23];
    char stringdata6[21];
    char stringdata7[23];
    char stringdata8[10];
    char stringdata9[26];
    char stringdata10[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_BookshelfView_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_BookshelfView_t qt_meta_stringdata_BookshelfView = {
    {
        QT_MOC_LITERAL(0, 13),  // "BookshelfView"
        QT_MOC_LITERAL(14, 20),  // "openProjectRequested"
        QT_MOC_LITERAL(35, 0),  // ""
        QT_MOC_LITERAL(36, 11),  // "BookProject"
        QT_MOC_LITERAL(48, 7),  // "project"
        QT_MOC_LITERAL(56, 22),  // "createProjectRequested"
        QT_MOC_LITERAL(79, 20),  // "loadProjectRequested"
        QT_MOC_LITERAL(100, 22),  // "deleteProjectRequested"
        QT_MOC_LITERAL(123, 9),  // "projectId"
        QT_MOC_LITERAL(133, 25),  // "duplicateProjectRequested"
        QT_MOC_LITERAL(159, 19)   // "exportJsonRequested"
    },
    "BookshelfView",
    "openProjectRequested",
    "",
    "BookProject",
    "project",
    "createProjectRequested",
    "loadProjectRequested",
    "deleteProjectRequested",
    "projectId",
    "duplicateProjectRequested",
    "exportJsonRequested"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_BookshelfView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       5,    0,   53,    2, 0x06,    3 /* Public */,
       6,    0,   54,    2, 0x06,    4 /* Public */,
       7,    1,   55,    2, 0x06,    5 /* Public */,
       9,    1,   58,    2, 0x06,    7 /* Public */,
      10,    1,   61,    2, 0x06,    9 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

Q_CONSTINIT const QMetaObject BookshelfView::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_BookshelfView.offsetsAndSizes,
    qt_meta_data_BookshelfView,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_BookshelfView_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BookshelfView, std::true_type>,
        // method 'openProjectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const BookProject &, std::false_type>,
        // method 'createProjectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadProjectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deleteProjectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'duplicateProjectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const BookProject &, std::false_type>,
        // method 'exportJsonRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const BookProject &, std::false_type>
    >,
    nullptr
} };

void BookshelfView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BookshelfView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->openProjectRequested((*reinterpret_cast< std::add_pointer_t<BookProject>>(_a[1]))); break;
        case 1: _t->createProjectRequested(); break;
        case 2: _t->loadProjectRequested(); break;
        case 3: _t->deleteProjectRequested((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->duplicateProjectRequested((*reinterpret_cast< std::add_pointer_t<BookProject>>(_a[1]))); break;
        case 5: _t->exportJsonRequested((*reinterpret_cast< std::add_pointer_t<BookProject>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BookshelfView::*)(const BookProject & );
            if (_t _q_method = &BookshelfView::openProjectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BookshelfView::*)();
            if (_t _q_method = &BookshelfView::createProjectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BookshelfView::*)();
            if (_t _q_method = &BookshelfView::loadProjectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BookshelfView::*)(const QString & );
            if (_t _q_method = &BookshelfView::deleteProjectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (BookshelfView::*)(const BookProject & );
            if (_t _q_method = &BookshelfView::duplicateProjectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (BookshelfView::*)(const BookProject & );
            if (_t _q_method = &BookshelfView::exportJsonRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *BookshelfView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BookshelfView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BookshelfView.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int BookshelfView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void BookshelfView::openProjectRequested(const BookProject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BookshelfView::createProjectRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BookshelfView::loadProjectRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void BookshelfView::deleteProjectRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void BookshelfView::duplicateProjectRequested(const BookProject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void BookshelfView::exportJsonRequested(const BookProject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
