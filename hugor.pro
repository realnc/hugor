QT += core svg widgets
QT_CONFIG -= no-pkg-config
TEMPLATE = app
CONFIG += silent warn_on link_pkgconfig strict_c++ c++1z gc_binaries
TARGET = hugor
ICON = mac_icon.icns

VERSION_MAJOR = 3
VERSION_MINOR = 0
VERSION_PATCH = 99
VERSION = "$$VERSION_MAJOR"."$$VERSION_MINOR"."$$VERSION_PATCH"
DEFINES += \
    HUGOR_VERSION=\\\"$$VERSION\\\" \
    HUGOR_VERSION_MAJOR=$$VERSION_MAJOR \
    HUGOR_VERSION_MINOR=$$VERSION_MINOR \
    HUGOR_VERSION_PATCH=$$VERSION_PATCH

lessThan(QT_MAJOR_VERSION, 5) {
    error(Qt 4 is not supported. You need at least Qt 5.5.)
}
equals(QT_MAJOR_VERSION, 5) {
    lessThan(QT_MINOR_VERSION, 5) {
        error(Qt 5.5 or higher is required. You are using Qt "$$QT_MAJOR_VERSION"."$$QT_MINOR_VERSION")
    }
    lessThan(QT_MINOR_VERSION, 6) {
        QMAKE_CXXFLAGS += -std=c++1z
    }
}

#SANITIZER_FLAGS = -fsanitize=undefined,integer -fno-omit-frame-pointer
#QMAKE_CFLAGS += $$SANITIZER_FLAGS
#QMAKE_CXXFLAGS += $$SANITIZER_FLAGS
#QMAKE_LFLAGS += $$SANITIZER_FLAGS

!system-boost {
    INCLUDEPATH += boost
}

!disable-audio {
    CONFIG(debug, release|debug) {
        DEFINES += AULIB_DEBUG
    }

    INCLUDEPATH += \
        SDL_audiolib \
        SDL_audiolib/3rdparty/fmt/include \
        SDL_audiolib/3rdparty/speex_resampler \
        SDL_audiolib/include \
        SDL_audiolib/src

    PKGCONFIG += sdl2 sndfile libmpg123 fluidsynth

    xmp {
        PKGCONFIG += libxmp
        DEFINES += USE_DEC_XMP=1
        SOURCES += SDL_audiolib/src/DecoderXmp.cpp
    }
    else:modplug {
        PKGCONFIG += libmodplug
        DEFINES += USE_DEC_MODPLUG=1
        SOURCES += SDL_audiolib/src/DecoderModplug.cpp
    }
    else {
        PKGCONFIG += libopenmpt
        DEFINES += USE_DEC_OPENMPT=1
        SOURCES += SDL_audiolib/src/DecoderOpenmpt.cpp
    }

    adlmidi {
        PKGCONFIG += libADLMIDI
        DEFINES += USE_DEC_ADLMIDI=1
        SOURCES += SDL_audiolib/src/DecoderAdlmidi.cpp
    }

    DEFINES += \
        AULIB_STATIC_DEFINE \
        FMT_HEADER_ONLY \
        OUTSIDE_SPEEX \
        RANDOM_PREFIX=SDL_audiolib \
        SPX_RESAMPLE_EXPORT=

    HEADERS += \
        $$files(SDL_audiolib/*.h) \
        $$files(SDL_audiolib/include/*.h) \
        $$files(SDL_audiolib/include/Aulib/*.h) \
        $$files(SDL_audiolib/src/*.h) \
        $$files(SDL_audiolib/src/missing/*.h) \
        src/oplvolumebooster.h \
        src/rwopsbundle.h \
        src/synthfactory.h

    SOURCES += \
        src/oplvolumebooster.cc \
        src/rwopsbundle.c \
        src/soundaulib.cc \
        src/synthfactory.cc \
        SDL_audiolib/3rdparty/speex_resampler/resample.c \
        SDL_audiolib/src/missing/sdl_load_file_rw.c \
        SDL_audiolib/src/Decoder.cpp \
        SDL_audiolib/src/DecoderFluidsynth.cpp \
        SDL_audiolib/src/DecoderMpg123.cpp \
        SDL_audiolib/src/DecoderSndfile.cpp \
        SDL_audiolib/src/Processor.cpp \
        SDL_audiolib/src/Resampler.cpp \
        SDL_audiolib/src/ResamplerSpeex.cpp \
        SDL_audiolib/src/Stream.cpp \
        SDL_audiolib/src/stream_p.cpp \
        SDL_audiolib/src/aulib.cpp \
        SDL_audiolib/src/sampleconv.cpp
} else {
    DEFINES += DISABLE_AUDIO
    SOURCES += src/soundnone.cc
}

disable-video {
    DEFINES += DISABLE_VIDEO
} else {
    disable-audio {
        error("Video support needs audio support to be enabled.")
    }

    PKGCONFIG += libvlc

    win32 {
        DEFINES += DL_VLC
    }

    HEADERS += \
        src/videoplayer.h \
        src/vlcaudiodecoder.h

    SOURCES += \
        src/videoplayer.cc \
        src/vlcaudiodecoder.cc
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
    gc_binaries:contains(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
        QMAKE_LFLAGS += -Wl,-dead_strip
    }
}

win32 {
    TARGET = Hugor
    RC_ICONS = w32_icon.ico
    QMAKE_TARGET_COMPANY = "Nikos Chantziaras"
    QMAKE_TARGET_DESCRIPTION = "Hugor - A Hugo Interpreter"
    QMAKE_TARGET_COPYRIGHT = "Copyright 2011, Kent Tessman; 2011-2022, Nikos Chantziaras"

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
    .github/workflows/main.yml \
    .clang-format \
    .gitattributes \
    .gitignore \
    LICENSE \
    example.cfg \
    hugo/LICENSE.TXT

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

linux {
    VLC_PREFIX = $$system(pkg-config libvlc --variable=prefix)

    appimage.target = appimage
    appimage.commands = \
        rm -f Hugor.AppImage \
        && rm -rf AppDir \
        && $$shell_quote($$QMAKE_QMAKE) PREFIX=$$shell_quote($$OUT_PWD/AppDir/usr) -config release -config adlmidi $$shell_quote($$_PRO_FILE_) \
        && make -j"$$QMAKE_HOST.cpu_count" \
        && make install \
        && mkdir -p $$shell_quote($$OUT_PWD/AppDir/usr/lib/vlc) \
        && cp -a $$shell_quote($$VLC_PREFIX/lib)/libvlc* $$shell_quote($$VLC_PREFIX/lib/vlc)/libvlc* $$shell_quote($$OUT_PWD/AppDir/usr/lib/) \
        && cp -a $$shell_quote($$VLC_PREFIX/lib/vlc/plugins) $$shell_quote($$OUT_PWD/AppDir/usr/lib/vlc/) \
        && rm -f $$shell_quote($$OUT_PWD/AppDir/usr/lib/vlc/plugins/plugins.dat) \
        && find $$shell_quote($$OUT_PWD/AppDir/usr/lib) -type f \\( -name $$shell_quote(*.la) -o -name $$shell_quote(*.a) \\) -exec rm $$shell_quote({}) \; \
        && patchelf --set-rpath $$shell_quote(\$$ORIGIN/../../../) $$shell_quote($$OUT_PWD/AppDir/usr/lib/vlc/plugins)/*/* \
        && linuxdeployqt \
            $$shell_quote($$OUT_PWD/AppDir/usr/share/applications/nikos.chantziaras.hugor.desktop) \
            -appimage \
            -no-copy-copyright-files \
            -no-translations \
            -qmake=$$shell_quote($$QMAKE_QMAKE) \
            -extra-plugins=iconengines,platformthemes

    QMAKE_EXTRA_TARGETS += appimage
}

macx {
    VLC_LIBDIR = $$system(pkg-config libvlc --variable=libdir)

    macdist.target = macdist
    macdist.commands = \
        rm -rf Hugor.app \
        && rm -f Hugor.zip \
        && $$shell_quote($$QMAKE_QMAKE) -config release -config adlmidi $$shell_quote($$_PRO_FILE_) \
        && make -j"$$QMAKE_HOST.cpu_count" \
        && mkdir -p Hugor.app/Contents/Frameworks/vlc \
        && cp -a $$shell_quote($$VLC_LIBDIR)/libvlc*.dylib Hugor.app/Contents/Frameworks/ \
        && cp -a $$shell_quote($$VLC_LIBDIR/vlc/plugins) Hugor.app/Contents/Frameworks/vlc/ \
        && rm -f Hugor.app/Contents/Frameworks/vlc/plugins/plugins.dat \
        && find Hugor.app/Contents/Frameworks/ -type f \\( -name $$shell_quote(*.la) -o -name $$shell_quote(*.a) \\) -exec rm $$shell_quote({}) \; \
        && $$shell_quote($$dirname(QMAKE_QMAKE)/macdeployqt) Hugor.app -verbose=2 \
        && ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 Hugor.app Hugor.zip

    legacymacdist.target = legacymacdist
    legacymacdist.commands = \
        rm -rf Hugor.app \
        && rm -f Hugor.zip \
        && $$shell_quote($$QMAKE_QMAKE) -config release -config adlmidi $$shell_quote($$_PRO_FILE_) \
        && make -j"$$QMAKE_HOST.cpu_count" \
        && sed -i \'\' \'s/\$${MACOSX_DEPLOYMENT_TARGET}/10.9/g\' Hugor.app/Contents/Info.plist \
        && dylibbundler -x Hugor.app/Contents/MacOS/Hugor -b -cd -d Hugor.app/Contents/Frameworks -p '@rpath' -s $$shell_quote($$VLC_LIBDIR) \
        && install_name_tool -add_rpath '@executable_path/../Frameworks/' Hugor.app/Contents/MacOS/Hugor \
        && rm -f Hugor.app/Contents/Frameworks/libvlc.* Hugor.app/Contents/Frameworks/libvlccore.* \
        && mkdir -p Hugor.app/Contents/Frameworks/vlc \
        && cp -a $$shell_quote($$VLC_LIBDIR)/libvlc*.dylib Hugor.app/Contents/Frameworks/ \
        && cp -a $$shell_quote($$VLC_LIBDIR/vlc/plugins) Hugor.app/Contents/Frameworks/vlc/ \
        && rm -f Hugor.app/Contents/Frameworks/vlc/plugins/plugins.dat \
        && find Hugor.app/Contents/Frameworks/ -type f \\( -name $$shell_quote(*.la) -o -name $$shell_quote(*.a) \\) -exec rm $$shell_quote({}) \; \
        && strip Hugor.app/Contents/MacOS/Hugor \
        && find Hugor.app/Contents/Frameworks/ -type f -name $$shell_quote(*.dylib) -exec strip -x $$shell_quote({}) \; \
        && ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 Hugor.app Hugor.zip

    QMAKE_EXTRA_TARGETS += macdist legacymacdist
}

win32 {
    VLC_LIBDIR = $$(VLC_LIBDIR)
    isEmpty(VLC_LIBDIR) {
        message("VLC_LIBDIR environment variable not set or is empty, will use /opt/vlc")
        VLC_LIBDIR = /opt/vlc
    }

    windist.target = windist
    windist.commands = \
        rm -rf release \
        && rm -f $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}.zip) \
        && $$shell_quote($$QMAKE_QMAKE) -config release -config adlmidi $$shell_quote($$_PRO_FILE_) \
        && make -j"$$QMAKE_HOST.cpu_count" \
        && mkdir -p $$shell_quote(release/Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/plugins) \
        && cd release \
        && cp -a $$shell_quote($${VLC_LIBDIR})/*.dll $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/) \
        && cp -a $$shell_quote($${VLC_LIBDIR}/plugins) $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/) \
        && rm -f $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/plugins/plugins.dat) \
        && find $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/) -type f \\( -name $$shell_quote(*.la) -o -name $$shell_quote(*.a) \\) -exec rm $$shell_quote({}) \; \
        && cp -a Hugor.exe $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}/) \
        && zip -r -9 $$shell_quote(../Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR}.zip) $$shell_quote(Hugor-$${VERSION_MAJOR}.$${VERSION_MINOR})

    QMAKE_EXTRA_TARGETS += windist
}
