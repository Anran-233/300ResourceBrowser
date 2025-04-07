QT       += core gui

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

INCLUDEPATH += \
    $${PWD}/define \
    $${PWD}/form 

SOURCES += \
    define/config.cpp \
    define/cpatch.cpp \
    define/tool.cpp \
	form/data_item.cpp \
	form/data_main.cpp \
    form/dialog_set.cpp \
    form/form_bank.cpp \
    form/form_dat.cpp \
	form/form_image.cpp \
    form/form_reader.cpp \
	form/loading.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    define/config.h \
    define/cpatch.h \
	define/img_decode.h \
    define/tool.h \
	form/data_item.h \
	form/data_main.h \
    form/dialog_set.h \
    form/form_bank.h \
    form/form_dat.h \
	form/form_image.h \
    form/form_reader.h \
	form/loading.h \
    mainwindow.h

FORMS += \
	form/data_item.ui \
	form/data_main.ui \
    form/dialog_set.ui \
    form/form_bank.ui \
    form/form_dat.ui \
	form/form_image.ui \
    form/form_reader.ui \
	form/loading.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

# 版本信息
VERSION = 1.0.5.0
DEFINES += APP_VERSION=\"\\\"$${VERSION}\\\"\"
# 文件图标
RC_ICONS += \
    ico_exe.ico \
    ico_jmp.ico \
    ico_bank.ico \
    ico_dat.ico
# 公司名称
QMAKE_TARGET_COMPANY = "Anran233���޿ɰ���˾"
# 产品名称
QMAKE_TARGET_PRODUCT = "300Ӣ�� ��Դ����� ���װ�"
# 文件说明
QMAKE_TARGET_DESCRIPTION = "��300Ӣ�ۡ���Ϸ��Դ�����"
# 版权信息
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2021-2023 Anran233."
# 中文（简体）
RC_LANG = 0x0004

