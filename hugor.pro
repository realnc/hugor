QT += core gui
TEMPLATE = app
CONFIG += warn_on silent
VERSION = 0.8.0.0
TARGET = hugor
ICON = mac_icon.icns
RC_FILE += hugor.rc


!sound_sdl:!sound_fmod {
    error("Use CONFIG+=sound_sdl or CONFIG+=sound_fmod to select a sound engine")
}

sound_sdl:sound_fmod {
    error("Choose either sound_sdl or sound_fmod, but not both")
}

sound_sdl:DEFINES += SOUND_SDL
sound_fmod:DEFINES += SOUND_FMOD

# On Windows and OS X we build static binaries, so we need to explicitly
# include the text codec plugins.
win32|macx {
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs
}

macx {
    TARGET = Hugor
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
    QMAKE_CFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_CXXFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_LFLAGS += -dead_strip

    sound_sdl {
        QMAKE_LFLAGS += -F./Frameworks
        LIBS += -framework SDL_mixer -framework SDL
        INCLUDEPATH += \
            ./Frameworks/SDL.framework/Headers \
            ./Frameworks/SDL_mixer.framework/Headers \
            ./Frameworks/smpeg.framework/Headers
    }

    sound_fmod {
        LIBS += -L./fmod/api/lib -lfmodex
        INCLUDEPATH += ./fmod/api/inc
    }
} else {
    sound_sdl {
        CONFIG += link_pkgconfig
        PKGCONFIG += SDL_mixer
    }

    sound_fmod {
        contains(QMAKE_HOST.arch, x86_64) {
            LIBS += -lfmodex64
        } else {
            LIBS += -lfmodex32
        }
    }
}

win32 {
    TARGET = Hugor

    *-g++* {
        QMAKE_CFLAGS += -mtune=generic -march=i686
        QMAKE_CXXFLAGS += -mtune=generic -march=i686

        # Dead code stripping (requires patched binutils).
        QMAKE_CFLAGS += -fdata-sections -ffunction-sections
        QMAKE_CXXFLAGS += -fdata-sections -ffunction-sections
        QMAKE_LFLAGS += -Wl,--gc-sections
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

sound_sdl:SOURCES += src/soundsdl.cc
sound_fmod:SOURCES += src/soundfmod.cc

OTHER_FILES += \
    README \
    README.linux-bin
