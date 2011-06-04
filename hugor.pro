QT += core gui
TEMPLATE = app
CONFIG += warn_on
VERSION = 0.1.0.99
TARGET = hugor

macx {
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_LFLAGS += -F./Frameworks
    LIBS += -framework SDL_mixer -framework SDL
    INCLUDEPATH += \
        ./Frameworks/SDL.framework/Headers \
        ./Frameworks/SDL_mixer.framework/Headers \
        ./Frameworks/smpeg.framework/Headers
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.5.sdk
    TARGET = Hugor
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += sdl
    # Normally we would use pkg-config for SDL_mixer too, but it has to appear
    # in the linker flags before SDL_sound, which lacks pkg-config support, or
    # else we crash.
    LIBS += -lSDL_mixer
}
win32 {
    LIBS += -lvorbisfile -lvorbis -logg
    TARGET = Hugor
}

# This is just a hack to make code completion work OK in Qt Creator.
INCLUDEPATH += /usr/include/SDL
INCLUDEPATH -= /usr/include/SDL

# We use warn_off to allow only default warnings, not to supress them all.
QMAKE_CXXFLAGS_WARN_OFF =
QMAKE_CFLAGS_WARN_OFF =

*-g++* {
    # Avoid a flood of "unused parameter" warnings.
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
}

INCLUDEPATH += src hugo
OBJECTS_DIR = obj
MOC_DIR = tmp
UI_DIR = tmp

DEFINES += QT

RESOURCES += resources.qrc

FORMS += \
    src/aboutdialog.ui \
    src/confdialog.ui

HEADERS += \
    src/aboutdialog.h \
    src/confdialog.h \
    src/happlication.h \
    src/hdispitem.h \
    src/hframe.h \
    src/hmainwindow.h \
    src/hugodefs.h \
    src/kcolorbutton.h \
    src/settings.h \
    \
    hugo/heheader.h \
    hugo/htokens.h

SOURCES += \
    src/aboutdialog.cc \
    src/confdialog.cc \
    src/happlication.cc \
    src/heqt.cc \
    src/hframe.cc \
    src/hmainwindow.cc \
    src/kcolorbutton.cc \
    src/main.cc \
    src/settings.cc \
    \
    #hugo/heblank.c \
    #hugo/iotest.c \
    hugo/he.c \
    hugo/hebuffer.c \
    hugo/heexpr.c \
    hugo/hemisc.c \
    hugo/heobject.c \
    hugo/heparse.c \
    hugo/heres.c \
    hugo/herun.c \
    hugo/heset.c \
    hugo/stringfn.c
