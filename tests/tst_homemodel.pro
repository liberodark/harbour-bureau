TARGET = tst_homemodel
QT = core
CONFIG += console link_pkgconfig c++11
PKGCONFIG += mlite5
SOURCES += tst_homemodel.cpp ../src/appmodel.cpp ../src/homemodel.cpp ../src/desktopparser.cpp
HEADERS += ../src/appmodel.h ../src/homemodel.h ../src/desktopparser.h ../src/appinfo.h
