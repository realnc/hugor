#! /bin/bash

# This is just my personal AppImage build script. It bundles and patches
# libVLC and its plugins so that they work from within the AppImage.
# linuxdeployqt can not do this automatically since libVLC loads the plugins
# at runtime.
#
# Warning: It will most probably not work anywhere else than my own build
# machine. You will probably need to adapt it for your own use. If you do,
# make sure patchelf is at least version 0.10. Earlier versions will not
# work.

QT_PATH=/home/realnc/opt/qt-5.12.2
VLC_LIBS=/usr/local/lib
BUILD_DIR=/tmp/hugorbuild

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"/..

rm -rf "$BUILD_DIR" \
&& mkdir -p "$BUILD_DIR" \
&& cd "$BUILD_DIR" \
&& "$QT_PATH"/bin/qmake -config adlmidi PREFIX="$PWD"/AppDir/usr "$PROJECT_DIR" \
&& make -j$(nproc) \
&& make install \
&& mkdir -p AppDir/usr/lib/vlc \
&& cp -a "$VLC_LIBS"/libvlc* "$VLC_LIBS"/vlc/libvlc* AppDir/usr/lib/ \
&& cp -a "$VLC_LIBS"/vlc/plugins AppDir/usr/lib/vlc/ \
&& rm -f AppDir/usr/lib/vlc/plugins/plugins.dat \
&& find AppDir/usr/lib -type f \( -name "*.la" -o -name "*.a" \) -exec rm '{}' \; \
&& patchelf --set-rpath '$ORIGIN/../../../' AppDir/usr/lib/vlc/plugins/*/* \
&& "$VLC_LIBS"/vlc/vlc-cache-gen AppDir/usr/lib/vlc/plugins \
&& linuxdeployqt-continuous-x86_64.AppImage \
    AppDir/usr/share/applications/nikos.chantziaras.hugor.desktop \
    -appimage \
    -no-copy-copyright-files \
    -no-translations \
    -qmake="$QT_PATH"/bin/qmake \
    -extra-plugins=iconengines,platformthemes \
&& mv *.AppImage "$PROJECT_DIR/Hugor.AppImage" \
&& rm -rf "$BUILD_DIR"
