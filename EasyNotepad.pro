#-------------------------------------------------
#
# Project created by QtCreator 2014-12-20T09:10:01
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
greaterThan(QT_MAJOR_VERSION, 6): QT += multimedia

TARGET = EasyNotepad
TEMPLATE = app

# Include Git Hash / Revision Details
# Uncomment NOGIT if Git not used.
#DEFINES += NOGIT
GITHASH = \\\"$$system(git --git-dir=\"$$PWD/.git\" describe --always --tags)\\\"
LIBHASH = \\\"$$system(git --git-dir=\"$$PWD/../Lib/.git\" describe --always --tags)\\\"
DEFINES += GITHASH=$$GITHASH
DEFINES += LIBHASH=$$LIBHASH
DEFINES += PWD=\\\"$$PWD\\\"

win32:RC_FILE=icon.rc
win32:LIBS += -lUser32

SOURCES += main.cpp\
        mainwindow.cpp \
    filenavigator.cpp \
    fileselect.cpp \
    listviewstrings.cpp \
    ../Lib/warningform.cpp \
    ../Lib/warningok.cpp \
    ../Lib/warningyesno.cpp \
    ../Lib/supportfunctions.cpp \
    globalsearch.cpp \
    ../Lib/itemselect.cpp \
    ../Lib/safetextedit.cpp \
    ../Lib/lineeditnavigator.cpp \
    help.cpp \
    importfilter.cpp \
    ../Lib/alertsound.cpp \
    ../Lib/iniconfig.cpp \
    formatselect.cpp \
    ../Lib/aes.cpp \
    ../Lib/encryption.cpp \
    ../Lib/safelineedit.cpp

HEADERS  += mainwindow.h \
    filenavigator.h \
    fileselect.h \
    listviewstrings.h \
    ../Lib/warningform.h \
    ../Lib/warningok.h \
    ../Lib/warningyesno.h \
    ../Lib/supportfunctions.h \
    globalsearch.h \
    filetypes.h \
    ../Lib/itemselect.h \
    ../Lib/safetextedit.h \
    ../Lib/lineeditnavigator.h \
    help.h \
    importfilter.h \
    ../Lib/alertsound.h \
    ../Lib/iniconfig.h \
    formatselect.h \
    ../Lib/aes.h \
    ../Lib/encryption.h \
    ../Lib/safelineedit.h

FORMS    += mainwindow.ui \
    fileselect.ui \
    ../Lib/warningform.ui \
    ../Lib/warningok.ui \
    ../Lib/warningyesno.ui \
    globalsearch.ui \
    ../Lib/itemselect.ui \
    help.ui \
    formatselect.ui \
    ../Lib/encryption.ui

RESOURCES += \
    ../Lib/sounds.qrc \
    Icon.qrc

OTHER_FILES +=
