# -------------------------------------------------
# Project created by QtCreator 2011-07-22T11:33:45
# -------------------------------------------------
QT += core \
    gui \
    xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pgsconverter
CONFIG -= console
#CONFIG -= app_bundle
TEMPLATE = app
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/editpointdialog.cpp \
    src/aboutdialog.cpp \
    src/pointmodel.cpp \
    src/globaldefs.cpp \
    src/safe_bin.cpp \
    src/configdialog.cpp
HEADERS += src/mainwindow.h \
    src/editpointdialog.h \
    src/aboutdialog.h \
    src/pointmodel.h \
    src/globaldefs.h \
    src/safe_bin.h \
    src/configdialog.h
FORMS += ui/mainwindow.ui \
    ui/editpointdialog.ui \
    ui/aboutdialog.ui \
    ui/configdialog.ui
INCLUDEPATH += src
UI_DIR = tmp/.ui
MOC_DIR = tmp/.moc
OBJECTS_DIR = tmp/.obj
RCC_DIR = tmp/.rcc
RESOURCES += images.qrc

RC_FILE = pgsconverter_resource.rc 

VERSION=2.0.1
