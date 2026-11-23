TARGET = qmlcheck
QT += quick qml
CONFIG += link_pkgconfig c++11
PKGCONFIG += mlite5
SOURCES += qmlcheck.cpp ../src/appmodel.cpp ../src/homemodel.cpp ../src/desktopparser.cpp ../src/systembridge.cpp
HEADERS += ../src/appmodel.h ../src/homemodel.h ../src/desktopparser.h ../src/appinfo.h ../src/systembridge.h
