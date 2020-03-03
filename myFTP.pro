#-------------------------------------------------
#
# Project created by QtCreator 2016-07-10T00:49:54
#
# 修正内容：对于mainwindow测试代码：
#       初始化是返回按钮，使初始化使为false
#       下载文件时，如果选择的是目录则进行报错。
#-------------------------------------------------

QT       += core gui
QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = myFTP
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qftp.cpp \
    qurlinfo.cpp

HEADERS  += mainwindow.h \
    qftp.h \
    qurlinfo.h

FORMS    += mainwindow.ui
