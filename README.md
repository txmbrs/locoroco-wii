# LocoRoco Wii — preview

Fanowski port demonstracyjny pierwszego poziomu na Wii. Dane gry są generowane
lokalnie przez konwerter z własnej kopii LocoRoco na PSP. Repozytorium nie
zawiera obrazu gry ani wyodrębnionych z niego zasobów.

## Instalacja

1. Pobierz paczkę konwertera z zakładki Releases i rozpakuj ją na Windows.
2. Umieść obok `KONWERTUJ.bat` jedną własną kopię gry PSP jako ISO, CSO v1
   lub folder `PSP_GAME` i uruchom `KONWERTUJ.bat`.
3. Skopiuj `output/apps/LocoRocoWii` do folderu `apps` na karcie SD Wii i
   uruchom aplikację przez Homebrew Channel.

Preview oparto na europejskiej wersji UCES00304; inne regiony nie są potwierdzone.
Kod fizyki i niektóre sceny są adaptacją, a nie odzyskanym silnikiem PSP.
Nie udostępniaj publicznie wygenerowanego folderu `output`, który zawiera dane gry.

## Budowanie silnika Wii

Wymagane jest środowisko devkitPro z devkitPPC, libogc, libfat i Box2D 2.4.1.
Oryginalna kompilacja używała libogc 2.6.0, libfat-ogc 1.2.0 i devkitPPC r45.
Przykład z CMake w środowisku devkitPro:

```sh
cmake -S wii -B build-wii -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Wii.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build-wii
```

Silnik uruchamia się z danymi utworzonymi przez konwerter; sam kod źródłowy
nie zawiera poziomu ani grafiki i dźwięków gry.

## Źródła i licencje

`wii/` zawiera kod silnika, a `converter/` kod importera i parser. Informacje
o bibliotekach oraz ich licencje znajdują się w `converter/licenses/` i
`converter/native_parser/`. Licencja MIT w `converter/LICENSE` dotyczy importera.
Stan prawny i pochodzenie pozostałego kodu silnika opisuje
`converter/licenses/WII-I-DEKODERY/POCHODZENIE-2026-11-05.md`.
Wykorzystano bibliotekę Box2D, libogc/libfat/WiiUse oraz narzędzia vgmstream
i FFmpeg. Jedną z inspiracji i wcześniejszych baz projektu był remake Lua
[dispensablegames/locoroco](https://github.com/dispensablegames/locoroco).
LocoRoco i materiały gry należą do ich odpowiednich właścicieli. Projekt jest
nieoficjalny i niezwiązany z właścicielami marki.
