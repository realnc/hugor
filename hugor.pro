QT += core
contains(QT_MAJOR_VERSION, 5):QT += widgets
TEMPLATE = app
CONFIG += warn_on link_pkgconfig exceptions
VERSION = 1.0.0.0
TARGET = hugor
ICON = mac_icon.icns
RC_FILE += hugor.rc

PKGCONFIG += SDL_mixer

contains(QT_MAJOR_VERSION, 4) {
    PKGCONFIG += \
        QtGStreamer-0.10 \
        QtGStreamerUi-0.10 \
        QtGStreamerUtils-0.10 \
        gstreamer-video-0.10

    HEADERS += \
        src/videoplayergst_p.h \
        src/rwopsappsrc.h

    SOURCES += \
        src/videoplayergst.cc \
        src/videoplayergst_p.cc \
        src/rwopsappsrc.cc
} else {
    QT += multimediawidgets

    HEADERS += \
        src/videoplayerqt5_p.h \
        src/rwopsqiodev.h

    SOURCES += \
        src/videoplayerqt5.cc \
        src/videoplayerqt5_p.cc \
        src/rwopsqiodev.cc
}

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
}

win32 {
    TARGET = Hugor
    QTPLUGIN += dsengine

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
    src/soundsdl.cc \
    src/rwopsbundle.c \
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
