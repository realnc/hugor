QT += core widgets
QT_CONFIG -= no-pkg-config
TEMPLATE = app
CONFIG += silent warn_on link_pkgconfig strict_c++ c++14 gc_binaries
TARGET = hugor
ICON = mac_icon.icns

VERSION_MAJOR = 2
VERSION_MINOR = 2
VERSION_PATCH = 99
VERSION = "$$VERSION_MAJOR"."$$VERSION_MINOR"."$$VERSION_PATCH"
DEFINES += HUGOR_VERSION=\\\"$$VERSION\\\"

lessThan(QT_MAJOR_VERSION, 5) {
    error(Qt 4 is not supported. You need at least Qt 5.5.)
}
contains(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 5) {
    error(Qt 5.5 or higher is required. You are using Qt "$$QT_MAJOR_VERSION"."$$QT_MINOR_VERSION")
}

#SANITIZER_FLAGS = -fsanitize=undefined,integer -fno-omit-frame-pointer
#QMAKE_CFLAGS += $$SANITIZER_FLAGS
#QMAKE_CXXFLAGS += $$SANITIZER_FLAGS
#QMAKE_LFLAGS += $$SANITIZER_FLAGS

!disable-audio {
    CONFIG(debug, release|debug) {
        DEFINES += AULIB_DEBUG
    }

    SOURCES *= src/rwopsbundle.c
    INCLUDEPATH += \
        SDL_audiolib \
        SDL_audiolib/include \
        SDL_audiolib/resampler \
        SDL_audiolib/src \
        SDL_audiolib/src/missing

    PKGCONFIG += sdl2 sndfile libmpg123 fluidsynth

    xmp {
        PKGCONFIG += libxmp
        DEFINES += USE_DEC_XMP=1
        SOURCES += SDL_audiolib/src/AudioDecoderXmp.cpp
    }
    else:modplug {
        PKGCONFIG += libmodplug
        DEFINES += USE_DEC_MODPLUG=1
        SOURCES += SDL_audiolib/src/AudioDecoderModplug.cpp
    }
    else {
        PKGCONFIG += libopenmpt
        DEFINES += USE_DEC_OPENMPT=1
        SOURCES += SDL_audiolib/src/AudioDecoderOpenmpt.cpp
    }

    adlmidi {
        PKGCONFIG += libADLMIDI
        DEFINES += USE_DEC_ADLMIDI=1
        SOURCES += SDL_audiolib/src/AudioDecoderAdlmidi.cpp
    }

    DEFINES += \
        AULIB_STATIC_DEFINE \
        SPX_RESAMPLE_EXPORT= \
        RANDOM_PREFIX=SDL_audiolib \
        OUTSIDE_SPEEX

    HEADERS += \
        $$files(SDL_audiolib/include/*.h) \
        $$files(SDL_audiolib/include/Aulib/*.h) \
        $$files(SDL_audiolib/src/*.h) \
        $$files(SDL_audiolib/src/missing/*.h) \
        src/oplvolumebooster.h \
        src/synthfactory.h

    SOURCES += \
        src/soundaulib.cc \
        src/oplvolumebooster.cc \
        src/synthfactory.cc \
        SDL_audiolib/resampler/resample.c \
        SDL_audiolib/src/missing/sdl_load_file_rw.c \
        SDL_audiolib/src/AudioDecoder.cpp \
        SDL_audiolib/src/AudioDecoderFluidsynth.cpp \
        SDL_audiolib/src/AudioDecoderMpg123.cpp \
        SDL_audiolib/src/AudioDecoderSndfile.cpp \
        SDL_audiolib/src/AudioResampler.cpp \
        SDL_audiolib/src/AudioResamplerSpeex.cpp \
        SDL_audiolib/src/AudioStream.cpp \
        SDL_audiolib/src/Processor.cpp \
        SDL_audiolib/src/Stream.cpp \
        SDL_audiolib/src/audiostream_p.cpp \
        SDL_audiolib/src/aulib.cpp \
        SDL_audiolib/src/sampleconv.cpp
} else {
    DEFINES += DISABLE_AUDIO
    SOURCES += src/soundnone.cc
}

!disable-video {
    # We still need SDL for SDL_RWops, even without audio.
    disable-audio {
        PKGCONFIG += sdl2
    }
    gstreamer-1 | gstreamer-0 {
        DEFINES += VIDEO_GSTREAMER

        gstreamer-0 {
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
            src/videoplayergst.cc \
            src/videoplayergst_p.cc
    }
    else:qt5-video {
        DEFINES += VIDEO_QT5

        QT += multimediawidgets

        HEADERS += \
            src/videoplayerqt5_p.h \
            src/rwopsqiodev.h

        SOURCES += \
            src/videoplayerqt5.cc \
            src/videoplayerqt5_p.cc \
            src/rwopsqiodev.cc
    }
    else {
        disable-audio {
            error("The libVLC video backend can't be built without audio support.")
        }
        PKGCONFIG += libvlc
        DEFINES += VIDEO_VLC
        win32 {
            DEFINES += DL_VLC
        }
        HEADERS += \
            src/videoplayervlc_p.h \
            src/vlcaudiodecoder.h
        SOURCES += \
            src/videoplayervlc.cc \
            src/vlcaudiodecoder.cc
    }
    HEADERS += src/videoplayer.h
    SOURCES *= src/rwopsbundle.c
} else {
    DEFINES += DISABLE_VIDEO
}

macx {
    TARGET = Hugor
    QMAKE_INFO_PLIST = Info.plist
    HEADERS += src/macos.h
    SOURCES += src/macos.mm
    LIBS += -framework AppKit
    contains(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 10) {
        message("*** You are using Qt < 5.10. You need to manually specify")
        message("*** LSMinimumSystemVersion in the generated Info.plist in")
        message("*** the app bundle after building.")
    }
}

win32 {
    TARGET = Hugor
    RC_ICONS = w32_icon.ico
    QMAKE_TARGET_COMPANY = "Nikos Chantziaras"
    QMAKE_TARGET_DESCRIPTION = "Hugor - A Hugo Interpreter"
    QMAKE_TARGET_COPYRIGHT = "Copyright 2006, Kent Tessman; 2011-2019, Nikos Chantziaras"

    !disable-video {
        qt5-video {
            QTPLUGIN += dsengine
        }
    }

    *-g++* {
        QMAKE_CFLAGS += -march=i686 -mtune=generic
        QMAKE_CXXFLAGS += -march=i686 -mtune=generic
    }
}

# We use warn_off to allow only default warnings, not to supress them all.
QMAKE_CXXFLAGS_WARN_OFF =
QMAKE_CFLAGS_WARN_OFF =

*-g++*|*-clang* {
    # Avoid "unused parameter" warnings with C code.
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
}

INCLUDEPATH += src hugo
OBJECTS_DIR = obj
MOC_DIR = tmp
UI_DIR = tmp

DEFINES += HUGOR QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x050600
CONFIG(debug, debug|release) {
    DEFINES += BOOST_CB_ENABLE_DEBUG=1
}

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
    src/rwopsbundle.h \
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
    src/hugorfile.cc \
    src/util.cc \
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

appdataxml.files = desktop/nikos.chantziaras.hugor.appdata.xml
desktopfile.files = desktop/nikos.chantziaras.hugor.desktop
desktopicon.files = desktop/hugor.png
docs.files = NEWS README
fileicons.files = desktop/hicolor
mimefile.files = desktop/hugor.xml

OTHER_FILES *= \
    .clang-format \
    .gitattributes \
    .gitignore \
    .travis.yml \
    LICENSE \
    example.cfg \
    hugo/LICENSE.TXT \
    scripts/build_appimage.sh \
    scripts/build_macos.sh

isEmpty(PREFIX) {
    PREFIX = /usr/local
}
isEmpty(BINDIR) {
    BINDIR = "$$PREFIX/bin"
}
isEmpty(DATADIR) {
    DATADIR = "$$PREFIX/share"
}
isEmpty(DOCDIR) {
    DOCDIR = "$$DATADIR/doc/$$TARGET-$$VERSION"
}

appdataxml.path = "$$DATADIR/metainfo"
desktopfile.path = "$$DATADIR/applications"
desktopicon.path = "$$DATADIR/icons/hicolor/256x256/apps"
docs.path = "$$DOCDIR"
fileicons.path = "$$DATADIR/icons"
mimefile.path = "$$DATADIR/mime/packages"
target.path = "$$BINDIR"

INSTALLS += appdataxml desktopfile desktopicon docs fileicons mimefile target
