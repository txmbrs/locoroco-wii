# LocoRoco Wii preview converter

Use the prebuilt Windows package from the project's Releases page. Place one
LocoRoco 1 PSP ISO, CSO v1, or `PSP_GAME` folder next to `CONVERT.bat`, then
run the batch file. After conversion, copy `output/apps/LocoRocoWii` to the
`apps` folder on your Wii storage and launch it through the Homebrew Channel.

The converter works locally and does not download game data. Its output includes
assets extracted from your game copy; do not redistribute that output. The
preview conversion has been checked with the European UCES00304 edition; other
regions and gameplay on a Wii were not verified for this build.

The Wii engine, parser, and converter each have their own provenance. See
`THIRD_PARTY_NOTICES.md` and the licenses distributed with the release.
