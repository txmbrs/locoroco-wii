# Pochodzenie kodu i bibliotek audio

## Dekodery z aktualnej paczki

Wszystkie 11 dołączonych DLL audio są identyczne bajt po bajcie z plikami `ext_libs/dll-x64` oficjalnego repozytorium vgmstream commit `764c84c5048932054356f2ea67a71ea7673abc83`. Konkretne sumy SHA-256 są w `DLL-ORIGIN-SHA256.txt`. Oznaczenie EXE wskazuje ten sam commit vgmstream, ale identyczność EXE z oficjalną kompilacją nie została wykazana.

Repozytorium tego commitu dostarcza teksty licencji komponentów w `ext_libs/licenses`, skopiowane do `converter/licenses/VGMSTREAM-DEPS`. Dokumentacja tego samego commitu wskazuje FFmpeg n5.1.2, libvorbis 1.3.7 i libogg 1.3.5, mpg123 1.32.0 / 1.31.1, Opus 1.3.1, Speex 1.2.1, CELT 0.6.1 i 0.11.0 oraz LibAtrac9. Dokumentacja zastrzega, że gotowe DLL mogą być starsze od zalecanych wersji; same nazwy plików licencji nie dowodzą pełnego pochodzenia źródeł. W czterech DLL FFmpeg występuje napis `FFmpeg version n5.1.2`. Oficjalny kod n5.1.2 pobrano z https://github.com/FFmpeg/FFmpeg/tree/n5.1.2, commit `eacfcbae690f914a4b1b4ad06999f138540cc3d8`. Nie udowodniono, czy przy budowie dołączonych DLL były użyte dodatkowe poprawki. Jest również włączony libopus; komplet dokładnych źródeł i możliwość ich odtworzenia pozostają do dopięcia przed dystrybucją.

### G.719

Dokumentacja vgmstream wskazuje https://github.com/kode54/libg719_decode commit `da90ad8a676876c6c47889bcea6a753f9bbf7a73` oraz określa jego licencję jako nieznaną. W tym kodzie `g719.c` wymienia Christophera Snowhilla i zastrzega prawa, a pliki `reference_code/` wskazują Ericsson AB oraz Polycom Inc. Obecność repozytorium w internecie nie ustala warunków dalszej dystrybucji wszystkich jego części. Nie ustalono właściwej licencji dołączonej biblioteki; nie należy oznaczać jej jako MIT ani GPL. Obecny CLI importuje DLL bezpośrednio. Wariant CLI budowany bez G.719 jest osobnym eksperymentem i wymaga sprawdzenia działania przed zastąpieniem aktualnego.

Powstał także eksperymentalny zestaw `publication/audio-build/decoder-psp-experimental`: samodzielnie zbudowany vgmstream bez G.719 oraz osobno zbudowany FFmpeg n5.1.2 i DLL winpthreads z MSYS2. Zawiera tylko zależności potrzebne w tym wariancie; obecność plików i symboli importowanych bibliotek sprawdzono statycznie, bez uruchamiania. Skonfigurowano dekodery ATRAC3 i ATRAC3plus oraz wbudowane w vgmstream PS-ADPCM. Kompletna konwersja, dźwięk i inne wydania gry nie zostały zweryfikowane. Archiwa źródłowe tych trzech składników znajdują się w `publication/audio-build`.

## Pochodzenie kodu portu

- Aktualny kod to `publication/native-map-engine/source`; jego część pochodzi z kopii `../locoroco-wii_source_backup`, która nie zawiera danych o autorach lub licencji. Na przykład `AssetManager.cpp`, `IsoReader.cpp` są identyczne, a `Loco.cpp` i `Game.cpp` były rozwijane na tej podstawie. Brak historii Git portu nie pozwala udowodnić pełnego autorstwa.
- Zachowana kopia `../locoroco-src` wskazuje repozytorium https://github.com/dispensablegames/locoroco. W lokalnym drzewie nie znaleziono pliku licencji. Stary `Loco.cpp` zawiera analogiczne wzory jak `loco.lua` remake'u: 12 + rozmiar/2 dla liczby elementów i sqrt(1000 * rozmiar) dla promienia. Model i nazwy klas różnią się; podobieństwo nie dowodzi samo, że cały kod został skopiowany. Zapis `FIXES_2026-10-23.md` potwierdza korzystanie z remake'u jako punktu odniesienia. Trzeba ustalić prawa i zakres ewentualnej adaptacji; nie można nadać praw do cudzego kodu samym dodaniem pliku LICENSE.
- Parser `converter/native_parser/blv.ts` i `texture_format.ts` wskazuje autora petton-svn i noclip.website PR 893; zachowano dołączony plik `native_parser/LICENSE` oraz NOTICE, w tym zastrzeżenie o reverse engineeringu.
- Aktywny silnik linkuje Box2D, libogc/libfat i WiiUse. W wersji libogc 2.6.0 kod `wiiuse/wiiboard.c` wskazuje GPL-3.0-or-later; symbole tego pliku widoczne są w ELF. Licencja importera MIT nie obejmuje automatycznie starego ani nowego kodu silnika. Wybranie GPL dla własnych części wymaga uprzedniego potwierdzenia praw do ich udostępnienia i zgodności wszystkich wykorzystanych części.

Wnioski są techniczną dokumentacją pochodzenia, nie potwierdzeniem praw do nazwy, oryginalnej gry czy wszystkich wykorzystywanych implementacji. Pliki z ROM-u, grafiki i dźwięki z niego wyciągnięte nie są objęte licencjami kodu.
