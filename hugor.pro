QT += core gui
contains(QT_MAJOR_VERSION, 5):QT += widgets
TEMPLATE = app
CONFIG += warn_on link_pkgconfig exceptions
VERSION = 1.0.0.0
TARGET = hugor
ICON = mac_icon.icns
RC_FILE += hugor.rc

PKGCONFIG += QtGStreamer-0.10 QtGStreamerUi-0.10 QtGStreamerUtils-0.10

!count(SOUND, 1) {
    error("Use SOUND=sdl or SOUND=fmod to select a sound engine")
} else:!contains(SOUND, sdl):!contains(SOUND, fmod) {
    error("SOUND argument not recognized")
}

CONFIG += $$SOUND

sdl:DEFINES += SOUND_SDL
fmod:DEFINES += SOUND_FMOD

# Static OS X builds need to explicitly include the text codec plugins.
macx {
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs
}

macx {
    TARGET = Hugor
    QMAKE_INFO_PLIST = Info.plist
    exists(/Developer/SDKs/MacOSX10.5.sdk) {
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
        QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
    } else {
        warning("OS X 10.5 SDK not found. Will use Qt defaults.")
    }
    QMAKE_CFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_CXXFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_LFLAGS += -dead_strip

    sdl {
        PKGCONFIG += SDL_mixer
    }

    fmod {
        LIBS += -L./fmod/api/lib -lfmodex
        INCLUDEPATH += ./fmod/api/inc
    }
} else {
    sdl {
        PKGCONFIG += SDL_mixer
    }

    fmod {
        LIBS += -L/opt/fmodex/api/lib -lfmodex
        INCLUDEPATH += /opt/fmodex/api/inc
    }
}

win32 {
    TARGET = Hugor

    *-g++* {
        QMAKE_CFLAGS += -march=i686 -mtune=generic
        QMAKE_CXXFLAGS += -march=i686 -mtune=generic

        # Dead code stripping (requires patched binutils).
        QMAKE_CFLAGS += -fdata-sections -ffunction-sections
        QMAKE_CXXFLAGS += -fdata-sections -ffunction-sections
        QMAKE_LFLAGS += -Wl,--gc-sections

        # Don't dead-strip the resource section (it contains the icon,
        # version strings, etc.)  We use a linker script to do that.
        QMAKE_LFLAGS += $$PWD/w32_linkscript
    }
}


# We use warn_off to allow only default warnings, not to supress them all.
QMAKE_CXXFLAGS_WARN_OFF =
QMAKE_CFLAGS_WARN_OFF =

*-g++* {
    # Avoid "unused parameter" warnings with C code.
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
}

INCLUDEPATH += /usr/local/include
INCLUDEPATH += src hugo
OBJECTS_DIR = obj
MOC_DIR = tmp
UI_DIR = tmp

DEFINES += HUGOR

RESOURCES += resources.qrc

FORMS += \
    src/aboutdialog.ui \
    src/confdialog.ui

HEADERS += \
    src/aboutdialog.h \
    src/confdialog.h \
    src/happlication.h \
    src/heqtheader.h \
    src/hframe.h \
    src/hmainwindow.h \
    src/hmarginwidget.h \
    src/hscrollback.h \
    src/hugodefs.h \
    src/kcolorbutton.h \
    src/settings.h \
    src/settingsoverrides.h \
    src/version.h \
    src/gstvideoplayer.h \
    src/rwopsappsrc.h \
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
    src/hmarginwidget.cc \
    src/hscrollback.cc \
    src/kcolorbutton.cc \
    src/main.cc \
    src/settings.cc \
    src/settingsoverrides.cc \
    src/gstvideoplayer.cc \
    src/rwopsappsrc.cc \
    \
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

sdl {
    HEADERS += src/rwopsbundle.h

    SOURCES += \
        src/soundsdl.cc \
        src/rwopsbundle.c
}

fmod:SOURCES += src/soundfmod.cc

OTHER_FILES += \
    README \
    README.linux-bin \
    w32_linkscript
