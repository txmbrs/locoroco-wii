# Przegląd licencji kandydata 1.16.1 — 29.10.2026

Zakres: pliki kandydata publication/release-candidate-1.16.1-20261029-100239, jego konwerter EXE, zewnętrzne DLL oraz biblioteki skonsolidowane w DOL. Ten przegląd nie ustala praw do marki i materiałów gry ani nie jest opinią prawną. Nie uruchamiano konwertera ani gry; analizowano pliki i dokumentację. Obecny ZIP nie został zmieniony.

## Najważniejsze ustalenia

### Dekodery

1. vgmstream-cli: lokalny COPYING jest dołączony, zawiera własną licencję permisywną. Osadzony tekst wersji EXE: `vgmstream CLI decoder r2117-286-g764c84c5 (Sep 14 2026)`. Samo COPYING vgmstream nie jest licencją zewnętrznych bibliotek DLL.
2. FFmpeg: 4 pliki DLL `avcodec-vgmstream-59`, `avformat-vgmstream-59`, `avutil-vgmstream-57`, `swresample-vgmstream-4` zawierają tekst LGPL 2.1 lub nowszej. Znaleziono tekst konfiguracji i zapisano go w ffmpeg-configuration-embedded.txt. Konfiguracja ma --enable-shared, --enable-libopus i nie zawiera --enable-gpl ani --enable-nonfree. To dowód o osadzonym opisie kompilacji, nie pełny dowód kompletnej zgodności ani pochodzenia źródła i wszystkich składowych. Wersje nazw DLL pasują do wariantu opisanego w starszej dokumentacji vgmstream jako FFmpeg n5.1.2, ale samych dokładnych źródeł tych binarek, łat i opcji nie potwierdzono.
3. libmpg123-0.dll: w dokumentacji vgmstream biblioteka opisana jest jako LGPL 2.1; pochodzenie i dokładna wersja używanej DLL nie zostały ustalone. Potrzebne są prawidłowe informacje o licencji i źródłach odpowiadających dystrybuowanej binarce.
4. libg719_decode.dll: dokumentacja vgmstream opisuje licencję tego dekodera jako nieznaną. Pole importów naszego vgmstream-cli zawiera `libg719_decode.dll` jako bezpośrednią zależność PE. Nie należy zakładać, że samo usunięcie DLL pozwoli uruchomić CLI; najbezpieczniejsza techniczna droga do rozważenia to ponowny build CLI bez G.719, z zachowaniem potrzebnych dekoderów PSP. Funkcjonalność trzeba później potwierdzić na własnej kopii gry.
5. libvorbis (w dokumentacji opisany wraz z libogg) — BSD 3-clause; libcelt 0.6.1 i 0.11.0 — w dokumentacji określono MIT; libspeex — wariant BSD; LibAtrac9 — MIT według starszej dokumentacji. Same nazwy binarek nie potwierdzają ich dokładnych rewizji ani tego, że wszystkie wymagane teksty i informacje są w ZIP-ie. FFmpeg ma wbudowaną obsługę libopus — prawa i notices tej zależności także wymagają ustalenia dla konkretnego builda.

Źródła: https://ffmpeg.org/legal.html ; https://github.com/vgmstream/vgmstream/blob/master/doc/BUILD-LIB.md ; https://github.com/vgmstream/vgmstream/blob/master/doc/BUILD.md ; https://github.com/bnnm/vgmstream-test/blob/main/doc/BUILD.md ; https://github.com/kode54/libg719_decode . Dokumenty o aktualnych lub starszych gałęziach są kontekstem, a nie dowodem dokładnego pochodzenia naszych DLL.

## Silnik Wii

W link.txt silnik używa archiwów statycznych: box2d, wiiuse, asnd, bte, fat, ogc, C++ runtime. Kontener switchchannel-builder:20240417 zawiera `ppc-box2d 2.4.1-2`, `libogc 2.6.0-1`, `libfat-ogc 1.2.0-1`, `devkitPPC r45-1`. To rzeczywiste pakiety użyte do budowania, nie przypuszczenie według nazwy.

- Box2D 2.4.1: MIT, kopia BOX2D-2.4.1-LICENSE.txt pochodzi z lokalnego tests/deps/box2d-2.4.1/LICENSE, zgodna z https://github.com/erincatto/box2d/blob/v2.4.1/LICENSE. W ostatnim ZIP-ie nie ma tej informacji.
- libogc 2.6.0: zainstalowany libogc_license.txt ma warunki typu zlib. Kopia w LIBOGC-2.6.0-LICENSE.txt. Nie oznacza to, że wszystkie podbiblioteki pakietu mają tylko tę licencję.
- libfat-ogc 1.2.0: fat.h zawiera warunki typu BSD z obowiązkiem podania informacji przy dystrybucji binarnej; w LIBFAT-1.2.0-HEADER.txt zachowano początkowy fragment zainstalowanego nagłówka. Należy przejrzeć odpowiednią wersję pozostałych plików źródłowych i dołączyć komplet notices. https://github.com/devkitPro/libfat/blob/master/source/libfat.c
- WiiUse: paczka libogc zawiera i linkuje libwiiuse.a; `nm` ELF potwierdza m.in. `wii_board_handshake`, `wii_board_event`, `wiiuse_*`, `WPAD_*`. W obecnym źródle devkitPro `wiiuse/wiiboard.c` widnieje GPL v3 lub późniejsza; zob. https://github.com/devkitPro/libogc/blob/master/wiiuse/wiiboard.c . To mocny sygnał, że należy sprawdzić dokładny kod libogc 2.6.0-1 i jego warunki oraz konsekwencje statycznego linkowania; nie wolno uznać całego libwiiuse za zlib tylko z ogólnego libogc_license.txt. Odrębny projekt wiiuse/wiiuse ma własną historię licencyjną, nie należy zakładać automatycznej tożsamości każdej wersji. Przy zastosowaniu GPL może być potrzebne udostępnienie odpowiedniego kodu źródłowego i tekstów licencji; warunki i dopuszczalne warianty należy ustalić dla konkretnej wersji.
- GCC/libstdc++: GNU opisuje GPLv3 z GCC Runtime Library Exception, ale trzeba zachować warunki poszczególnych faktycznie zlinkowanych bibliotek środowiska: https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html .
- Własny silnik: converter/LICENSE wyraźnie dotyczy tylko importera. Nie stwierdzono na tej podstawie, że wszystkie źródła silnika mają tę samą licencję; pochodzenie, autorstwo i uprawnienia należy ustalić oddzielnie. Podobnie prawa do marki, muzyki i grafiki gry są inną kwestią niż licencje narzędzi.

## Przed wydaniem

- Nie traktować kandydata ZIP jako gotowego pod względem licencji. Zidentyfikować pochodzenie i pełne warunki każdej zawartej biblioteki, dołączyć wymagane notices oraz źródła/odpowiednie materiały LGPL dla konkretnych buildów. FFmpeg opisuje własną checklistę zgodności: https://ffmpeg.org/legal.html .
- Ustalić status G.719; rozważyć własny build vgmstream bez tego składnika i z udokumentowanym źródłem i konfiguracją pozostałych kodeków.
- Ustalić źródło i licencje wszystkich włączonych fragmentów libogc 2.6.0 i libfat 1.2.0, w szczególności WiiUse; dołączyć Box2D MIT i libfat notices. Przygotować odpowiedni kod/warunki dystrybucji dla części objętych GPL, jeśli ta licencja się stosuje.
- Ustalić prawa do własnego kodu portu i wykorzystania IP gry. Udostępnienie ROM-u przez użytkownika nie rozstrzyga praw do dystrybucji portu ani znaków towarowych.
