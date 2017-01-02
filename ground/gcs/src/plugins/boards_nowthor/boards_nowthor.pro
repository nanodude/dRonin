TEMPLATE = lib
TARGET = Nowthor
include(../../gcsplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)

OTHER_FILES += Nowthor.json

HEADERS += \
    nowthorplugin.h \
    nakedsparky.h

SOURCES += \
    nowthorplugin.cpp \
    nakedsparky.cpp

RESOURCES += \
    nowthor.qrc

FORMS += \
