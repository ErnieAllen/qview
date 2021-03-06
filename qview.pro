#-------------------------------------------------
#
# Project created by QtCreator 2011-04-29T19:26:41
#
#-------------------------------------------------

QT       += core gui

TARGET = qview
TEMPLATE = app

SOURCES += main.cpp\
    model-queue.cpp \
    model-header.cpp \
    qmf-thread.cpp \
    dialogopen.cpp \
    dialogabout.cpp \
    dialogpurge.cpp \
    queuetableview.cpp \
    dialogcopy.cpp

HEADERS  += \
    main.h \
    model-queue.h \
    model-header.h \
    qmf-thread.h \
    dialogopen.h \
    dialogabout.h \
    dialogpurge.h \
    queuetableview.h \
    dialogcopy.h

FORMS    += \
    qview_main.ui \
    dialogopen.ui \
    dialogabout.ui \
    dialogpurge.ui \
    dialogcopy.ui

OTHER_FILES += \
    license.txt \
    README.txt \ 
    queue.xsd \
    todo.txt

RESOURCES += \
    toolbar_icons.qrc
