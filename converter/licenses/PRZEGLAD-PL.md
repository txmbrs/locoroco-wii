# Licencje i pochodzenie składników — kandydat 1.16.1

To zestawienie dotyczy konwertera i silnika Wii w tej paczce. Nie jest potwierdzeniem, że paczka spełnia już wszystkie warunki dystrybucji. Informacje o nieustalonym pochodzeniu pozostają otwarte.

## Konwerter i narzędzia

| Składnik | Informacje i lokalizacja |
|---|---|
| Importer | MIT, patrz `../LICENSE-importer.txt`; dotyczy importera, nie całego silnika Wii. |
| Parser noclip | `../native_parser/LICENSE` i `../native_parser/NOTICE.md`. |
| Python, Pillow, NumPy, PyInstaller | Teksty licencji w tym katalogu: `PYTHON_LICENSE.txt`, `Pillow/`, `numpy/`, `PYINSTALLER_COPYING.txt`. |
| Node.js | `../tools/NODE_LICENSE.txt`. |
| vgmstream CLI | `../tools/vgmstream/COPYING`; biblioteki wymienione niżej mają własne licencje. |
| FFmpeg: avcodec, avformat, avutil, swresample | DLL są identyczne z plikami vgmstream 764c84c5 i zawierają oznaczenie n5.1.2 oraz LGPL 2.1 lub późniejsza. Teksty w `VGMSTREAM-DEPS/`. Dokładne źródła z ewentualnymi poprawkami i komplet warunków dystrybucji nadal wymagają potwierdzenia. Konfiguracja: `WII-I-DEKODERY/ffmpeg-configuration-embedded.txt`. https://ffmpeg.org/legal.html |
| libmpg123 | Dokumentacja wskazuje LGPL 2.1; dokładne źródła i wersja tej DLL niepotwierdzone. https://www.mpg123.de/ |
| libvorbis, libspeex, libcelt 0.6.1 i 0.11.0, LibAtrac9 | Dokumentacja opisuje odpowiednio licencje typu BSD, BSD, MIT i MIT; dokładne wersje i komplet wymaganych informacji dla dołączonych plików wymagają potwierdzenia. https://github.com/vgmstream/vgmstream/blob/master/doc/BUILD-LIB.md |
| libopus | Konfiguracja FFmpeg wskazuje użycie libopus; sposób włączenia, konkretne źródła i informacje licencyjne do potwierdzenia. |
| libg719_decode | Licencja konkretnego dekodera nieustalona. Jest bezpośrednią zależnością CLI i nie należy po prostu usuwać DLL. Dystrybucja wymaga wyjaśnienia praw lub zastąpienia odpowiednio zbudowanym dekoderem. https://github.com/kode54/libg719_decode |

## Biblioteki statycznie dołączone do silnika Wii

| Składnik | Informacje i lokalizacja |
|---|---|
| Box2D 2.4.1 | MIT, tekst i autor: `WII-I-DEKODERY/BOX2D-2.4.1-LICENSE.txt`. |
| libogc 2.6.0 | Ogólne warunki typu zlib: `WII-I-DEKODERY/LIBOGC-2.6.0-LICENSE.txt`. Poszczególne podbiblioteki mogą mieć inne warunki. |
| libfat-ogc 1.2.0 | Informacja typu BSD z nagłówka zainstalowanej wersji: `WII-I-DEKODERY/LIBFAT-1.2.0-HEADER.txt`. Komplet informacji z pozostałych plików źródłowych należy sprawdzić. |
| WiiUse / pozostałe części libogc | Receptura libogc 2.6.0-1 wskazuje źródła v2.6.0, commit a0b4b5680944ee7c2ae1b7af63a721623c1a6b69. Zawarty tam wiiboard.c jest oznaczony GPL-3.0-or-later, a jego symbole wykryto w ELF. Tekst GPL i plik źródłowy są w `WII-I-DEKODERY/`. Należy ustalić warunki pozostałych linkowanych składników i zgodną licencję kodu portu. https://github.com/devkitPro/libogc/blob/v2.6.0/wiiuse/wiiboard.c |
| GCC/libstdc++ | GNU opisuje licencję GPL-3.0 z GCC Runtime Library Exception; należy sprawdzić warunki pozostałych użytych części środowiska. https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html |
| Kod silnika portu | Licencja i pochodzenie całego kodu silnika wymagają osobnego ustalenia. Licencja importera nie obejmuje go automatycznie. |

Szczegóły i ograniczenia audytu: `WII-I-DEKODERY/RAPORT-PL.md`. Prawa do gry, marki i danych z własnego ROM-u użytkownika są odrębne od licencji tych bibliotek.
