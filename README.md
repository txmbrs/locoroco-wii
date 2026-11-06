# LocoRoco Wii — Preview

An unofficial first-level preview for Wii. A Windows converter builds the game
data locally from your own copy of LocoRoco for PSP. This repository does not
include a game image or extracted game assets.

## Installation

1. Download and extract the converter package from [Releases](../../releases).
2. Place one LocoRoco 1 PSP ISO, CSO v1, or `PSP_GAME` folder next to
   `CONVERT.bat` and run the batch file. You can also drag an image onto it.
3. Copy `output/apps/LocoRocoWii` to the `apps` folder on your Wii SD card or
   USB drive, and launch the game through the Homebrew Channel.

The previous converter build was checked with the European UCES00304 edition.
The rebuilt English executable, other editions and this package's Wii playback
have not been verified. The physics and some
scenes are Wii adaptations, not the recovered PSP engine. The generated `output`
folder contains data from your game copy; please do not redistribute it.

## Building the Wii engine

Install devkitPro with devkitPPC, libogc, libfat and Box2D 2.4.1. The existing
build used libogc 2.6.0, libfat-ogc 1.2.0 and devkitPPC r45:

```sh
cmake -S wii -B build-wii -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Wii.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build-wii
```

The engine needs data produced by the converter. The source tree does not
include the game level, graphics or audio.

## Sources and licensing

`wii/` contains the Wii engine and `converter/` the importer and parser. See
`converter/THIRD_PARTY_NOTICES.md` and `converter/licenses/` for provenance and
third-party license notices. The MIT license in `converter/LICENSE` applies to
the importer; the licensing and provenance of the entire engine remain to be
established. An earlier reference was the [dispensablegames Lua remake](https://github.com/dispensablegames/locoroco).

LocoRoco and its original game assets belong to their respective owners. This is
an unofficial project and is not affiliated with the rights holders.
