TARGET = harbour-bureau

CONFIG += sailfishapp
PKGCONFIG += mlite5

isEmpty(VERSION): VERSION = 0.1.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    src/main.cpp \
    src/desktopparser.cpp \
    src/appmodel.cpp \
    src/homemodel.cpp \
    src/systembridge.cpp

HEADERS += \
    src/appinfo.h \
    src/desktopparser.h \
    src/appmodel.h \
    src/homemodel.h \
    src/systembridge.h

DISTFILES += \
    qml/harbour-bureau.qml \
    qml/cover/CoverPage.qml \
    qml/pages/HomePage.qml \
    qml/pages/AppPickerDialog.qml \
    qml/pages/SettingsPage.qml \
    qml/components/HomeIcon.qml \
    qml/components/FolderOverlay.qml \
    qml/components/Toast.qml \
    rpm/harbour-bureau.spec \
    rpm/harbour-bureau.changes \
    harbour-bureau.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172 256x256
