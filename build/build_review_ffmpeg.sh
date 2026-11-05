#!/usr/bin/env bash
set -eu
# Isolated experimental build. No installation or modification of converter/tools.
repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
src="$repo/publication/dependency-sources/ffmpeg-n5.1.2"
build="$repo/publication/audio-build/ffmpeg-nothreads"
prefix="$repo/publication/audio-build/ffmpeg-nothreads-install"
mkdir -p "$build" "$prefix"
cd "$build"
"$src/configure" --prefix="$prefix" --build-suffix=-vgmstream \
 --target-os=mingw32 --arch=x86_64 --enable-shared --disable-static \
 --disable-programs --disable-doc --disable-network --disable-autodetect \
 --disable-w32threads --disable-pthreads --disable-os2threads \
 --disable-everything --disable-asm --disable-avdevice --disable-avfilter \
 --disable-swscale --disable-postproc --enable-swresample \
 --enable-decoder=atrac3p,atrac3,pcm_s16le --enable-demuxer=wav,oma \
 --enable-protocol=file --disable-debug
make -j4
make install
