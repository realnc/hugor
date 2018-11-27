QT += core widgets
QT_CONFIG -= no-pkg-config
TEMPLATE = app
CONFIG += silent warn_on link_pkgconfig strict_c++ c++14
VERSION = 1.0.0.99
TARGET = hugor
ICON = mac_icon.icns
RC_FILE += hugor.rc

lessThan(QT_MAJOR_VERSION, 5) {
    error(Qt 5 is required to build this application.)
}

#SANITIZER_FLAGS = -fsanitize=undefined,integer -fno-omit-frame-pointer
#QMAKE_CFLAGS += $$SANITIZER_FLAGS
#QMAKE_CXXFLAGS += $$SANITIZER_FLAGS
#QMAKE_LFLAGS += $$SANITIZER_FLAGS

# qmake on Qt 5.3 and lower doesn't recognize c++14
contains(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 4) {
    CONFIG += c++11
    QMAKE_CXXFLAGS_CXX11 = $$replace(QMAKE_CXXFLAGS_CXX11, "std=c\+\+11", "std=c++1y")
    QMAKE_CXXFLAGS_CXX11 = $$replace(QMAKE_CXXFLAGS_CXX11, "std=c\+\+0x", "std=c++1y")
}

static:DEFINES += STATIC_QT

!disable-audio {
    SOURCES *= src/rwopsbundle.c
    audiolib {
        INCLUDEPATH += \
            SDL_audiolib \
            SDL_audiolib/include \
            SDL_audiolib/resampler \
            SDL_audiolib/src

        PKGCONFIG += sdl2 sndfile libmpg123 fluidsynth libopenmpt

        DEFINES += \
            AULIB_STATIC_DEFINE \
            SPX_RESAMPLE_EXPORT= \
            RANDOM_PREFIX=SDL_audiolib \
            OUTSIDE_SPEEX \
            FLOATING_POINT

        SOURCES += \
            src/soundaulib.cc \
            SDL_audiolib/resampler/resample.c \
            SDL_audiolib/src/AudioDecoder.cpp \
            SDL_audiolib/src/AudioDecoderFluidsynth.cpp \
            SDL_audiolib/src/AudioDecoderOpenmpt.cpp \
            SDL_audiolib/src/AudioDecoderMpg123.cpp \
            SDL_audiolib/src/AudioDecoderSndfile.cpp \
            SDL_audiolib/src/AudioResampler.cpp \
            SDL_audiolib/src/AudioResamplerSpeex.cpp \
            SDL_audiolib/src/AudioStream.cpp \
            SDL_audiolib/src/Stream.cpp \
            SDL_audiolib/src/audiostream_p.cpp \
            SDL_audiolib/src/aulib.cpp \
            SDL_audiolib/src/sampleconv.cpp
    } else {
        sdl2 {
            PKGCONFIG += SDL2_mixer
        } else {
            PKGCONFIG += SDL_mixer
        }
        SOURCES += src/soundsdl.cc
    }
} else {
    DEFINES += DISABLE_AUDIO
    SOURCES += src/soundnone.cc
}

!disable-video {
    # We still need SDL for SDL_RWops, even without audio.
    disable-audio {
        sdl2 {
            PKGCONFIG += sdl2
        } else {
            PKGCONFIG += sdl
        }
    }
    qt5-video {
        DEFINES += VIDEO_QT5

        QT += multimediawidgets

        HEADERS += \
            src/videoplayerqt5_p.h \
            src/rwopsqiodev.h

        SOURCES += \
            src/videoplayerqt5.cc \
            src/videoplayerqt5_p.cc \
            src/rwopsqiodev.cc
    } else {
        DEFINES += VIDEO_GSTREAMER

        gstreamer-0.10 {
            PKGCONFIG += \
                gstreamer-interfaces-0.10 \
                gstreamer-video-0.10 \
                gstreamer-app-0.10
        } else {
            PKGCONFIG += \
                gstreamer-video-1.0 \
                gstreamer-app-1.0
        }

        HEADERS += \
            src/videoplayergst_p.h

        SOURCES += \
            src/gstinit.cc \
            src/videoplayergst.cc \
            src/videoplayergst_p.cc
    }
    SOURCES *= src/rwopsbundle.c
} else {
    DEFINES += DISABLE_VIDEO
}

macx {
    TARGET = Hugor
    QMAKE_INFO_PLIST = Info.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
    QMAKE_CFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_CXXFLAGS += -fvisibility=hidden -fomit-frame-pointer
    QMAKE_LFLAGS += -dead_strip
}

win32 {
    TARGET = Hugor
    !disable-video {
        gstreamer-0.10 {
            error("GStreamer 0.10 is not supported on Windows. You need GStreamer 1.x.")
        }
        qt5-video {
            QTPLUGIN += dsengine
        } else {
            include(gstreamer-static.pri)
        }
    }

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
    # Use optimizations that don't interfere with debugging in debug builds.
    QMAKE_CXXFLAGS_DEBUG += -Og
    QMAKE_CFLAGS_DEBUG += -Og
}

*-g++*|*-clang* {
    # Avoid "unused parameter" warnings with C code.
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
}

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
    src/rwopsbundle.h \
    src/videoplayer.h \
    src/enginerunner.h \
    src/hugohandlers.h \
    src/hugorfile.h \
    src/opcodeparser.h \
    src/util.h \
    src/extcolors.h \
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
    src/enginerunner.cc \
    src/hugohandlers.cc \
    src/opcodeparser.cc \
    src/extcolors.cc \
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

OTHER_FILES += \
    README \
    README.linux-bin \
    w32_linkscript
