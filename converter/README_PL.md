# Konwerter danych LocoRoco PSP do preview Wii

1. Umiesc jedna wlasna kopie LocoRoco 1 PSP (.iso, .cso v1 lub folder PSP_GAME)
   obok KONWERTUJ.bat. Nazwa obrazu jest dowolna. Nie musi nazywac sie locoroco.iso.
2. Uruchom KONWERTUJ.bat. Mozna tez przeciagnac obraz na ten plik.
3. Po komunikacie KONWERSJA DANYCH PREVIEW UKONCZONA skopiuj caly
   output/apps/LocoRocoWii do apps/LocoRocoWii na karcie SD/USB.

Zachowaj caly folder narzedzia: EXE, KONWERTUJ.bat i tools. EXE zawiera Python,
NumPy, Pillow, parsery i szablon silnika. W tools sa Node i vgmstream z bibliotekami.
Uzytkownik nie instaluje Pythona, Node, ffmpeg ani pakietow pip.
Narzedzie nie pobiera ROM-ow ani assetow z Internetu. Obraz jest tylko czytany.

## Co jest generowane

- boot.dol i meta.xml sa kopiowane z szablonu silnika bez danych gry.
- assets/animations: 23 klipy Kulche, 3 MuiMui i tory obiektow mapy,
  z oryginalnych pozycji/obrotow/skal i podmian tekstur. Czas nominalny 30 Hz,
  dla mapy uwzgledniony velocity_rate.
- assets/maps/st_flower01.lrcmap: geometria, UV, tekstury, kolizje, obiekty i wiatr PSP.
- assets/sprites/*.gx: obrazy z TIMP/BLV oryginalnego importu, przyciete lub zlozone
  do formatu renderera Wii. Wlasne teksty pomocy dla Wii sa generowane z fontu
  dolaczonego do Pillow; nie sa przechwyconymi obrazami z PSP.
- assets/audio/*.pcm: nazwane efekty z oryginalnego yel_locoroco.sgd i clear_jingle.
- assets/music: flower oraz cztery zsynchronizowane partie wokalne z yel_flower.
  Bank jest odczytywany bezposrednio z DATA.BIN, takze spoza blokow GPRS.
- assets/preview-assets.json: pochodzenie obrazow, dzwiekow i sumy danych.
- output/import-report.json i wii-preview-ready.json: stan konwersji i kopia zrodlowa.

Nie sa potrzebne nasze prywatne PNG/PCM, przechwycenia emulatora, stare naglowki
C++ z assetami ani locoroco.pak. Brak pliku wejsciowego przerywa konwersje;
nie ma awaryjnego korzystania ze starych danych.

## Zakres

To komplet danych wymaganych przez silnik preview swiata 1, nie pelny port
wszystkich zachowan oryginalu PSP. Fizyka postaci i kamera pozostaja kodem Wii.
Odtwarzanie znalezionych krzywych dziala w silniku; powiazanie wszystkich klipow
z nieodtworzonymi sygnalami i sekretami PSP nadal jest niepelne. Karta nagrody ma jawny
opis preview; nie udaje oryginalnej grafiki. Synteza sekwencji dzwiekowych nie
emuluje wszystkich kontrolerow/envelop i zapisuje pominiete operacje w raporcie.

Konwersje wykonano na lokalnej kopii UCES00304. Inne regiony moga wymagac
uzupelnienia obslugi. Nie uruchamiano gry ani testow automatycznych.

Ten folder jest roboczym narzedziem; nie przygotowano teraz wydania publicznego.
Wyniki output zawieraja dane gry uzytkownika i nie naleza do dystrybucji narzedzia.

## Zrodla narzedzi

Parser BLV/CLV: petton-svn, noclip.website PR 893 (MIT), licencja w native_parser.
Node: tools/NODE_LICENSE.txt. vgmstream i kodeki: tools/vgmstream/COPYING.
NumPy/Pillow oraz Python: ich licencje w licenses. Dane gry nie sa objete licencja
kodu konwertera.

## Zmiana 1.14.0

Uruchom ponownie konwerter i wgraj caly wygenerowany folder. Mapa ma format 2,
nowy silnik wymaga tez katalogu assets/animations. Stare dane 1.13 nie sa zamiennikiem.
Przeniesiono krzywe tła, wyspy i bramy, native model/klipy twarzy Kulche oraz MuiMui
na mapie. Klipy Kulche uruchamiaja: jedzenie (eat), wolanie (call), spokoj (stay),
rozgladanie (kyoro), otwarcie ust przy skoku (waaa) i glos (uta). Inne klipy sa
w banku do przyszlych stanow; nie zgadywano nieznanych wyzwalaczy silnika PSP.
Toczenie miekkiego ciala i przechyl +/-45 stopni nie zostaly zmienione.

Wersja 1.15.0: zdarzenia World 1
------------------------------
Konwerter tworzy dodatkowo assets/events/st_flower01.lrcevt (sygnały, MuiMui,
nagrody, przełączniki, waga i kryjówki). Mapa ma format 3. Wgraj cały folder
apps/LocoRocoWii, łącznie z nowym boot.dol i assets — pliki starszych wersji
nie są zamiennikami tych danych.

Silnik przechowuje odblokowane identyfikatory nagród w
apps/LocoRocoWii/save/flower01-rewards.bin. Konwerter nie usuwa tego zapisu.
Ponowne wejście do poziomu resetuje zdarzenia lokalne, zachowując nagrody.
Nie oznacza to jeszcze obsługi globalnego edytora Loco House ani przydziału
konkretnych numerowanych części domu. Osadzone skrypty scenek, część komend
kamery i dźwięków zdarzeń nadal nie są interpretowane. Raport eksportu zdarzeń
wymienia nieobsługiwane połączenia. Zachowanie na konsoli nie było uruchamiane.

Wersja 1.16.0: wymagane są nowe tekstury dialogów, dźwięki zdarzeń oraz
ponownie wyeksportowane pliki mapy i zdarzeń. Uruchom konwerter i kopiuj cały
folder aplikacji. Dźwięki i angielskie tekstury tutoriali są pobierane z PSP.
Fizyka Wii i część scen pozostają adaptacją; szczegóły w CHANGES_1.16.0.md.
