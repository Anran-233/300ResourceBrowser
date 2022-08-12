QT       += core gui
QT       += concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cpatch.cpp \
    dialog_preview.cpp \
    dialog_progress.cpp \
    dialog_set.cpp \
    dialog_tip.cpp \
    form_reader.cpp \
    main.cpp \
    mainwindow.cpp \
    widget_data.cpp \
    widget_hero.cpp \
    widget_local.cpp \
    widget_net.cpp

HEADERS += \
    ceventfilter.h \
    cpatch.h \
    dialog_preview.h \
    dialog_progress.h \
    dialog_set.h \
    dialog_tip.h \
    form_reader.h \
    img_decode.h \
    mainwindow.h \
    widget_data.h \
    widget_hero.h \
    widget_local.h \
    widget_net.h

FORMS += \
    dialog_preview.ui \
    dialog_progress.ui \
    dialog_set.ui \
    dialog_tip.ui \
    form_reader.ui \
    mainwindow.ui \
    widget_data.ui \
    widget_hero.ui \
    widget_local.ui \
    widget_net.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES += \
    300ResourceBrowser.rc

RC_FILE += 300ResourceBrowser.rc
