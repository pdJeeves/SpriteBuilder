#-------------------------------------------------
#
# Project created by QtCreator 2016-09-16T16:18:14
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SpriteBuilder
INCLUDEPATH += /home/anyuser/Downloads/squish-1.11/
LIBS += -L/home/anyuser/Downloads/squish-1.11/ -lsquish
TEMPLATE = app


SOURCES += main.cpp\
        spritebuilder.cpp \
    imageview.cpp \
    spritetable.cpp \
    importer.cpp \
    scaleimages.cpp \
    super_xbr.cpp \
    menuactions.cpp \
    commandchain.cpp \
    importsettings.cpp

HEADERS  += spritebuilder.h \
    imageview.h \
    byteswap.h \
    spritetable.h \
    commandchain.h \
    importsettings.h

FORMS    += spritebuilder.ui \
    importsettings.ui
