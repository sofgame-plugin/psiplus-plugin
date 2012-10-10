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
    cmdlineedit.h \
    settings.h \
    avatarframe.h \
    pluginhosts.h \
    specificenemiesview.h \
    aurainfo.h \
    equipitem.h \
    mainwidget.h \
    fightwidget.h \
    infowidget.h \
    thingswidget.h
SOURCES += sofgameplugin.cpp \
    plugin_core.cpp \
    main_window.cpp \
    fight.cpp \
    utils.cpp \
    sender.cpp \
    pers.cpp \
    pers_info.cpp \
    cmdlineedit.cpp \
    settings.cpp \
    avatarframe.cpp \
    pluginhosts.cpp \
    specificenemiesview.cpp \
    aurainfo.cpp \
    equipitem.cpp \
    mainwidget.cpp \
    fightwidget.cpp \
    infowidget.cpp \
    thingswidget.cpp
FORMS += main_window.ui \
    mainwidget.ui \
    fightwidget.ui \
    infowidget.ui \
    thingswidget.ui

# Resources
RESOURCES += sofgameplugin.qrc
include(aliases/aliases.pri)
include(thingstab/thingstab.pri)
include(thingftab/thingftab.pri)
include(subclasses/subclasses.pri)
include(textparsing/textparsing.pri)
include(maps/maps.pri)
include(statistic/statistic.pri)
