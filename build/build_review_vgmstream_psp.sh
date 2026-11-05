#!/usr/bin/env bash
set -eu
repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo/publication/audio-build/vgmstream-psp-src"
# For the examined LocoRoco banks only: PS-ADPCM is built in and ATRAC3plus
# uses FFmpeg. This is experimental until an actual conversion is checked.
make -j4 vgmstream_cli VGM_X64=1 VGM_G719=0 VGM_G7221=0 VGM_VORBIS=0 \
 VGM_MPEG=0 VGM_ATRAC9=0 VGM_CELT=0 VGM_SPEEX=0 VGM_FFMPEG=1 \
 EXTRA_CFLAGS=-std=gnu11
