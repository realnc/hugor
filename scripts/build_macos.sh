#! /bin/bash

# This is just my personal build script to correctly bundle libVLC. It will
# not work on anything else than my personal macOS build machine. macdeployqt
# can not do this automatically since libVLC loads the plugins at runtime.

QT_PATH="$HOME/Qt/5.12.2/clang_64"
VLC_LIBS="$HOME/opt/usr/lib"
BUILD_DIR=/tmp/hugorbuild
export PKG_CONFIG_PATH="$VLC_LIBS/pkgconfig"

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"/..

rm -rf "$BUILD_DIR" \
&& mkdir -p "$BUILD_DIR" \
&& cd "$BUILD_DIR" \
&& "$QT_PATH"/bin/qmake -config adlmidi "$PROJECT_DIR" \
&& make -j$(nproc) \
&& mkdir -p Hugor.app/Contents/Frameworks/vlc \
&& cp -a "$VLC_LIBS"/libvlc*.dylib Hugor.app/Contents/Frameworks/ \
&& cp -a "$VLC_LIBS"/vlc/plugins Hugor.app/Contents/Frameworks/vlc/ \
&& rm -f Hugor.app/Contents/Frameworks/vlc/plugins/plugins.dat \
&& find Hugor.app/Contents/Frameworks/ -type f \( -name "*.la" -o -name "*.a" \) -exec rm '{}' \; \
&& "$QT_PATH"/bin/macdeployqt Hugor.app \
&& LD_LIBRARY_PATH="$VLC_LIBS" "$VLC_LIBS"/vlc/vlc-cache-gen Hugor.app/Contents/Frameworks/vlc/plugins \
&& ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 Hugor.app Hugor.zip \
&& mv Hugor.zip "$PROJECT_DIR/" \
&& rm -rf "$BUILD_DIR"
