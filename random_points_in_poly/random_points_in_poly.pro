#-------------------------------------------------
#
# Project created by QtCreator 2015-09-20T14:54:19
#
#-------------------------------------------------

QT       -= core gui

TARGET = random_points_in_poly
CONFIG   += console
CONFIG   -= app_bundle

DESTDIR = $$PWD/bin

TEMPLATE = app

INCLUDEPATH += $$PWD/../../poly2tri/poly2tri
DEPENDPATH += $$PWD/../../poly2tri/poly2tri
LIBS += -L$$PWD/../../poly2tri/poly2tri/lib -lpoly2tri

INCLUDEPATH += /sztt-build/3rdparty/shapelib
DEPENDPATH += /sztt-build/3rdparty/shapelib
LIBS += -L/sztt-build/3rdparty/shapelib/win32 -lshapelib_i

SOURCES += \
    main.cpp
