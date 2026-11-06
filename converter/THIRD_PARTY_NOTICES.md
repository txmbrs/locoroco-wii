# Third-party notices and provenance

This document describes the preview's converter and Wii engine. The license in
`LICENSE-importer.txt` applies to the importer; it does not automatically license
the whole Wii engine or any original game content.

- **vgmstream**: built from commit `764c84c5048932054356f2ea67a71ea7673abc83`;
  its `COPYING` file is in `tools/vgmstream/`. The preview build disables G.719,
  G.722.1, Vorbis, MPEG, ATRAC9, CELT, and Speex and uses FFmpeg for ATRAC3plus.
- **FFmpeg**: shared libraries built from tag `n5.1.2`, commit
  `eacfcbae690f914a4b1b4ad06999f138540cc3d8`, with LGPL 2.1-or-later options.
  Source and build scripts are provided in `sources/`; the LGPL text is in
  `licenses/VGMSTREAM-DEPS/ffmpeg.COPYING.LGPLv2.1`.
- **winpthreads**: MSYS2 `mingw-w64-x86_64-winpthreads`
  `14.0.0.r569.g34eeafb7d-1`; its notices are in
  `licenses/WII-AND-DECODERS/WINPTHREADS-COPYING.txt`, with source in `sources/`.
- **Python, Pillow, NumPy, PyInstaller and Node.js**: license texts are under
  `licenses/` and `tools/NODE_LICENSE.txt`.
- **BLV/CLV parser**: petton-svn's implementation submitted to noclip.website
  [PR 893](https://github.com/magcius/noclip.website/pull/893). Its license and
  authorship notice are in `native_parser/`; the license includes an upstream
  note on reverse engineering.
- **Box2D 2.4.1**: MIT; see `licenses/WII-AND-DECODERS/BOX2D-2.4.1-LICENSE.txt`.
- **libogc 2.6.0 and libfat-ogc 1.2.0**: see their notices and source-file
  headers in `licenses/WII-AND-DECODERS/`. The linked WiiUse `wiiboard.c` in
  libogc 2.6.0 is marked GPL-3.0-or-later; its source and GPL text are included.
- **GCC runtime**: see `licenses/WII-AND-DECODERS/GCC-RUNTIME-EXCEPTION.txt`.

The engine includes code from an older Wii port whose complete authorship and
license have not been established. The [dispensablegames Lua remake](https://github.com/dispensablegames/locoroco)
was an earlier reference; similar formulas were found, but this alone does not
establish that the entire engine is copied. No repository-wide license was found
in the local copy of that remake. Rights to the game, name, and original assets
are separate from the licenses of these tools.

Earlier converter builds bundled eleven audio DLLs identical to the vgmstream
commit's `ext_libs/dll-x64` files. They included `libg719_decode.dll`, whose
license the upstream documentation describes as unknown. Those old binaries are
not part of this preview's decoder set. The initial preview conversion was
checked using a European UCES00304 game copy; console playback was not checked.
