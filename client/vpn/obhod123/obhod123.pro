#-------------------------------------------------
#
# Project created by QtCreator 2017-04-17T22:56:12
#
#-------------------------------------------------

QT       += widgets core gui network xml

TARGET = obhod123
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# Define api version
QMAKE_API_VERSION=1
DEFINES += "API_VERSION=\"$${QMAKE_API_VERSION}\""

include("../../../publib/publib.pri")


win32 {

#win32-g++ {
#   QMAKE_CXXFLAGS += -Werror
#}
#win32-msvc*{
#   QMAKE_CXXFLAGS += /WX
#}

FORMS    += ui/mainwindow.ui

RESOURCES += \
    res.qrc

OTHER_FILES += platform_win/vpnclient.rc
RC_FILE = platform_win/vpnclient.rc

HEADERS += publib/winhelp.h \
    backend/tapcontroller_win.h

SOURCES += publib/winhelp.cpp \
    backend/tapcontroller_win.cpp


CONFIG -= embed_manifest_exe
DEFINES += _CRT_SECURE_NO_WARNINGS VPNCLIENT_TAPSIGNED
#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

VERSION = 1.1.1.1
QMAKE_TARGET_COMPANY = "Obhod123"
QMAKE_TARGET_PRODUCT = "Obhod123"

CONFIG -= embed_manifest_exe

LIBS += -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32

# PRE_TARGETDEPS += $$PWD/deskmetrics/dma.lib
# LIBS += -L$$PWD/deskmetrics/ -ldma

INCLUDEPATH += $$PWD/deskmetrics
DEPENDPATH += $$PWD/deskmetrics


MT_PATH = \"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/bin/x64/mt.exe\"
WIN_PWD = $$replace(PWD, /, \\)
OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)

!win32-g++: QMAKE_POST_LINK = "$$MT_PATH -manifest $$quote($$WIN_PWD\\platform_win\\$$basename(TARGET).exe.manifest) -outputresource:$$quote($$OUT_PWD_WIN\\$(DESTDIR_TARGET);1)"
else: QMAKE_POST_LINK = "$$MT_PATH -manifest $$PWD/platform_win/$$basename(TARGET).exe.manifest -outputresource:$$OUT_PWD/$(DESTDIR_TARGET)"
#    QMAKE_POST_LINK = "$$MT_PATH -identity:\"obhod123.obhod123, version=2.6.13.1, processorArchitecture=x86, type=win32\" -outputresource:$$quote($$OUT_PWD_WIN\\$(DESTDIR_TARGET);1)"
}

macx {

OBJECTIVE_HEADERS +=
OBJECTIVE_SOURCES += publib/macos_functions.mm

HEADERS += \

SOURCES += \

QMAKE_OBJECTIVE_CFLAGS += -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/System/Library/Frameworks

FORMS    += ui/mainwindow_mac.ui

LIBS += -framework CoreServices -framework Foundation -framework AppKit

RESOURCES += \
    res_mac.qrc

ICON   = images/main.icns
}

SOURCES  += main.cpp\
            ui/mainwindow.cpp \
            ui/customshadoweffect.cpp \
    ui/serversmodel.cpp \
    ui/settingswindow.cpp \
    ui/flags.cpp \
    ui/Controls/SlidingStackedWidget.cpp \
    ui/Controls/serverstableview.cpp \
    publib/runguard.cpp \
    backend/api_http.cpp \
    backend/pinger.cpp \
    backend/vpnclient.cpp \
    backend/obhod123.cpp \
    backend/vpnutils.cpp \
    backend/updater.cpp \
    backend/router.cpp \
    backend/debug.cpp \
    backend/license.cpp \
    backend/settings.cpp \
    backend/networkcontroller_win.cpp

HEADERS  += ui/mainwindow.h \
            ui/customshadoweffect.h \
    ui/serversmodel.h \
    ui/Controls/SlidingStackedWidget.h \
    ui/Controls/serverstableview.h \
    ui/flags.h \
    publib/runguard.h \
    backend/pinger.h \
    backend/api_http.h \
    backend/vpnclient.h \
    backend/vpndefine.h \
    backend/obhod123.h \
    backend/vpnutils.h \
    backend/updater.h \
    backend/router.h \
    backend/debug.h \
    backend/license.h \
    backend/settings.h \
    backend/networkcontroller_win.h

FORMS    += ui/mainwindow.ui


TRANSLATIONS = translations/obhod123.en.ts \
    translations/obhod123.ru.ts
