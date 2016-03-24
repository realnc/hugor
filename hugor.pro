QT += core
QT_CONFIG -= no-pkg-config
contains(QT_MAJOR_VERSION, 5):QT += widgets
TEMPLATE = app
CONFIG += silent warn_on link_pkgconfig exceptions c++11
VERSION = 1.0.0.99
TARGET = hugor
ICON = mac_icon.icns
RC_FILE += hugor.rc

contains(QT_MAJOR_VERSION, 4):lessThan(QT_MINOR_VERSION, 6) {
    error(Qt $$QT_VERSION is not supported. You need at least version 4.6.)
}

static:DEFINES += STATIC_QT

!disable-audio {
    sdl2 {
        PKGCONFIG += SDL2_mixer
    } else {
        PKGCONFIG += SDL_mixer
    }
    SOURCES += src/soundsdl.cc
    SOURCES *= src/rwopsbundle.c
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

# Static OS X builds against Qt4 need to explicitly include the text codec plugins.
macx:static:contains(QT_MAJOR_VERSION, 4) {
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs
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
    # Avoid "unused parameter" warnings with C code.
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter

    # Use optimizations that don't interfere with debugging in debug builds.
    QMAKE_CXXFLAGS_DEBUG += -Og
    QMAKE_CFLAGS_DEBUG += -Og
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
