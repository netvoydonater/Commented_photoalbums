QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = commented_photoalbums
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        album.cpp \
        bydatestrategy.cpp \
        bydescriptionstrategy.cpp \
        bytagstrategy.cpp \
        logindialog.cpp \
        main.cpp \
        mainwindow.cpp \
        photo.cpp \
        photoeditdialog.cpp \
        photomanager.cpp \
        tag.cpp \
        user.cpp

HEADERS += \
        album.h \
        bydatestrategy.h \
        bydescriptionstrategy.h \
        bytagstrategy.h \
        logindialog.h \
        mainwindow.h \
        photo.h \
        photoeditdialog.h \
        photomanager.h \
        searchstrategy.h \
        tag.h \
        user.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
