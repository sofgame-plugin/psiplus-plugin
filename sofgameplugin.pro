CONFIG += release
include(../../psiplugin.pri)
HEADERS += sofgameplugin.h \
    plugin_core.h \
    main_window.h \
    common.h \
    fight.h \
    utils.h \
    sender.h \
    pers.h \
    pers_info.h \
    cmdlineedit.h
SOURCES += sofgameplugin.cpp \
    plugin_core.cpp \
    main_window.cpp \
    fight.cpp \
    utils.cpp \
    sender.cpp \
    pers.cpp \
    pers_info.cpp \
    cmdlineedit.cpp
FORMS += main_window.ui

# Resources
RESOURCES += sofgameplugin.qrc
include(aliases/aliases.pri)
include(thingstab/thingstab.pri)
include(thingftab/thingftab.pri)
include(subclasses/subclasses.pri)
include(textparsing/textparsing.pri)
include(maps/maps.pri)
