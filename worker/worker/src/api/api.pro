QT += core
QT -= gui

# Define api version
QMAKE_API_VERSION=1
DEFINES += "API_VERSION=$${QMAKE_API_VERSION}"


CONFIG += c++11 plugin
QMAKE_CFLAGS += -shared -fPIC -Ofast
QMAKE_CXXFLAGS += -Ofast

TARGET = api_v_$${QMAKE_API_VERSION}
TEMPLATE = lib

include($$PWD/../../../../publib/publib.pri)


SOURCES += \
    nx_interface.c \
    entity.cpp \
    api/client_requests.cpp \
    api/server_requests.cpp \
    api/docker_requests.cpp \
    router.cpp \
    api/management_requests.cpp \
    saveload.cpp


HEADERS += \
    nx_types.h \
    web_types.h \
    nx_interface.h \
    entity.h \
    api/client_requests.h \
    api/server_requests.h \
    api/docker_requests.h \
    router.h \
    fuckmacro.h \
    api/management_requests.h \
    saveload.h



