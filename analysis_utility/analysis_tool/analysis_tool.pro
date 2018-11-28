QT += widgets xml charts sql
QMAKE_CXXFLAGS += -std=gnu++11 -Wno-unused-parameter

HEADERS = $$files(src/*.h, true)
SOURCES = $$files(src/*.cpp, true)

INCLUDEPATH += src /usr/include/libusb-1.0/ ../../power_measurement_utility/mcu/common/

RESOURCES     = application.qrc

LIBS += -lusb-1.0

# install
target.path = /usr/bin/
INSTALLS += target

QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
