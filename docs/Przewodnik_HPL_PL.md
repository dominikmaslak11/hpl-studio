# Przewodnik po języku HPL (Orange5)

*Nauka języka skryptów programatora Orange5 na podstawie prawdziwych, jawnych skryptów `.hpl`.*

> Wersja robocza (PL). Wszystkie przykłady pochodzą z autentycznych plików z katalogu `HPL\` dystrybucji Orange5. Po zatwierdzeniu powstanie wersja angielska.

---

## Spis treści

1. [Wprowadzenie](#1-wprowadzenie)
2. [Anatomia skryptu](#2-anatomia-skryptu)
3. [Piny — PINO / PINI / PING](#3-piny)
4. [Rejestry i dane](#4-rejestry-i-dane)
5. [Liczby, stałe i makra](#5-liczby-stałe-i-makra)
6. [Operacje i matematyka](#6-operacje-i-matematyka)
7. [Sterowanie: pętle i warunki](#7-sterowanie-pętle-i-warunki)
8. [Podprogramy](#8-podprogramy)
9. [Komunikacja z użytkownikiem: PRINT i GET](#9-komunikacja-z-użytkownikiem)
10. [Standardowe sekcje operacji](#10-standardowe-sekcje-operacji)
11. [Studium przypadku 1 — I2C 24Cxx](#11-studium-przypadku-1--i2c-24cxx)
12. [Studium przypadku 2 — Microwire 93Cxx](#12-studium-przypadku-2--microwire-93cxx)
13. [Studium przypadku 3 — SPI 25Cxx](#13-studium-przypadku-3--spi-25cxx)
14. [Rejestracja i dystrybucja skryptów](#14-rejestracja-i-dystrybucja-skryptów)
15. [Dobre praktyki i debugowanie](#15-dobre-praktyki-i-debugowanie)
16. [Ściąga (dodatek)](#16-ściąga-dodatek)

---

## 1. Wprowadzenie

**HPL** (Hardware Programming Language) to język skryptów programatora **Orange5**. Skrypt HPL opisuje **jak sterować nóżkami gniazda** (bit po bicie), aby odczytać lub zapisać pamięć/mikrokontroler w podstawce ZIF.

Kluczowe fakty:

- Skrypt HPL to **zwykły plik tekstowy** `.hpl` — możesz go otworzyć i edytować w dowolnym edytorze.
- Program „bitbanguje" protokół: sam ustawiasz linie zegara i danych, wysyłasz bity i czytasz je z powrotem. HPL nie zna „gotowego I2C" — Ty budujesz protokół z pojedynczych stanów pinów.
- Pliki leżą w katalogu `HPL\` instalacji Orange5. Zaszyfrowane odpowiedniki `.hpx` to ta sama logika, tylko ukryta — do **tworzenia i rozpowszechniania własnych skryptów szyfrowanie nie jest potrzebne**.

Minimalny szkielet:

```hpl
; komentarz zaczyna się średnikiem
INFO="Opis chipu"
SOCKET=0

[Hello]
PRINT=("Hello World!")

[END]
```

---

## 2. Anatomia skryptu

Skrypt dzieli się na **nagłówek** (dyrektywy globalne) i **sekcje** w nawiasach kwadratowych `[NAZWA]`.

```hpl
; Orange programmer module v2.9
; (c) 1999-2011 CnCLab, pavel-pervomaysk
; CHIP=24C01 (128x8), 24C02 (256x8)

SOCKET=1 ;"I2C"          <- typ gniazda / trybu

PINO=WP, 2              <- definicje pinów (nagłówek)

CDELAY = 4 ; one set delay   <- globalne opóźnienie zegara

[!#SETUP]              <- sekcja specjalna: wykonywana raz przy wyborze
R10=0xA0

[INIT]                 <- standardowe sekcje operacji
...
[READ]
...
[WRITE]
...
[END]
```

| Element | Znaczenie |
|--------|-----------|
| `; ...` | komentarz do końca linii |
| `INFO="..."` | opis widoczny w GUI |
| `SOCKET=n` | typ gniazda/trybu (0 zwykłe, 1 I2C, 2 MW/Microwire, 4 SPI — wg komentarzy w skryptach) |
| `CDELAY=n` | ile „jednostek" trwa jeden stan zegara (reguluje szybkość) |
| `BAUD=n` | szybkość dla trybów szeregowych (UART/K-Line) |
| `VCC=mV` | napięcie zasilania w miliwoltach |
| `[NAZWA]` | sekcja — punkt wykonywania. Standardowe operacje mają ustalone nazwy (patrz rozdz. 10), a własne nazwy pojawiają się jako funkcje skryptu w GUI |

Sekcje o nazwach zaczynających się od `_` to **podprogramy** (rozdz. 8), a `[!SETUP]` / `[!#SETUP]` to kod inicjalizacyjny wykonywany przy wyborze chipu.

---

## 3. Piny

Definicja pinu przypisuje **nazwę** do **numeru nóżki** gniazda.

```hpl
PINO=SCK,0      ; wyjście na nóżce 0, dostępne jako "SCK"
PINO=#RW,9      ; wyjście ODWRÓCONE (znak # przed nazwą)
PINI=SO,1       ; wejście na nóżce 1
PING=SDA,1      ; pin dwukierunkowy (open-drain), typowe dla I2C
```

| Dyrektywa | Rola |
|-----------|------|
| `PINO=NAME,n` | pin **wyjściowy** — sterujesz nim |
| `PINI=NAME,n` | pin **wejściowy** — czytasz go |
| `PING=NAME,n` | pin **dwukierunkowy** (np. SDA/SCL w I2C, linie open-drain) |
| `PINO=#NAME,n` | wyjście **odwrócone** (logika ujemna) |

Sterowanie pinem:

```hpl
SCK=1          ; ustaw stan wysoki
SCK=0          ; ustaw stan niski
SCK=P          ; IMPULS zegara: robi 0→1→0 (jeden „takt")
SDA=R0[3]      ; ustaw pin z bitu 3 rejestru R0
DATA[I]=SDA    ; odczytaj pin do bitu I danej
```

`=P` to najczęstszy skrót — jeden impuls zegara. Kilka poleceń w jednej linii oddziela się przecinkami:

```hpl
SDA=1,SCL=1,SDA=0,SCL=0      ; sekwencja Start w I2C
```

---

## 4. Rejestry i dane

HPL udostępnia 16 rejestrów **32-bitowych**: `R0`–`RF` (czyli R0..R9, RA..RF; zapis szesnastkowy indeksu). Oraz zmienne specjalne:

| Nazwa | Znaczenie |
|-------|-----------|
| `R0`–`RF` | rejestry użytkownika (32-bit) |
| `DATA` | bajt/słowo bieżącej komórki (to, co czytasz/zapisujesz) |
| `ADR` | adres bieżącej komórki |
| `RA` | wynik okna `PRINT=A` (1 = OK, 0 = Anuluj) — uwaga: `RA` to też rejestr R10 |
| `I` | licznik pętli `LOOP` |

**Dostęp do bitu:** `R0[3]` to bit 3 rejestru `R0` (0 = najmłodszy). Działa też dla `DATA[I]`, `ADR[I]`.

```hpl
R0=0
R0[7]=1          ; ustaw bit 7  → R0 = 0x80
DATA[6]=SDA      ; wczytaj pin SDA do bitu 6 danej
```

**Aliasy rejestrów** — możesz nazwać rejestr w nagłówku:

```hpl
R10=I2CADR,H2    ; R10 nazywany "I2CADR", format wyświetlania: hex 2 cyfry
R9=STATUS,C8,WPEN,x,x,x,BL1,BL0,WEL,RDY   ; nazwa + nazwy bitów
```

Od tej pory `I2CADR` odnosi się do `R10`. Litera po przecinku to format podglądu: `H` = hex, `D`/`d` = dziesiętnie, `B` = binarnie, cyfra = liczba pozycji.

---

## 5. Liczby, stałe i makra

Trzy zapisy liczb — **wszystkie równoważne**:

```hpl
CONST=0x123B     ; szesnastkowo z prefiksem 0x
CONST=123BH      ; szesnastkowo z sufiksem H
CONST=1010B      ; binarnie z sufiksem B
R0=12345         ; dziesiętnie
```

**Makra** zaczynają się od `$` i zwykle definiowane są w `[!SETUP]`:

```hpl
[!#SETUP]
$WDELAY=25000     ; czas oczekiwania na zapis
```

Później używasz `$WDELAY` jak wartości: `P=$WDELAY`. Częste makra dostarczane przez środowisko: `$BLOCKSIZE` (rozmiar bloku bieżącej operacji).

`CONST=` służy też do wysłania stałej wartości bez rejestru.

---

## 6. Operacje i matematyka

Operacje działają „w miejscu" na rejestrze po lewej (`R0=+1` znaczy `R0 = R0 + 1`):

| Zapis | Działanie |
|-------|-----------|
| `R0=+1` `R0=-2` | dodaj / odejmij stałą |
| `R0=+R1` `R0=-R2` | dodaj / odejmij rejestr |
| `R0=&0x0FFF` | AND |
| `R0=\|1` | OR |
| `R0=^0xFF` | XOR |
| `R0=<<1` | przesunięcie w lewo |
| `R0=>>2` | przesunięcie w prawo |
| `R0=*R4` `R0=/R4` `R0=%R4` | mnożenie / dzielenie / modulo |
| `R7=R2` | kopia rejestru |
| `R0=^R3` | XOR z rejestrem |

Przykład (z `hpltest.hpl`):

```hpl
R1=0x1234800F
R0=^R1        ; XOR
R1=&0xFFFF    ; maska młodszego słowa
R1=>>8        ; przesuń
R1=|0xF030    ; ustaw bity
```

---

## 7. Sterowanie: pętle i warunki

### Pętle — `LOOP`

Dwie formy:

```hpl
LOOP=(7,0){ ... }        ; I biegnie 7,6,5,...,0  (w dół)
LOOP=(0,15){ ... }       ; I biegnie 0,1,...,15   (w górę)
LOOP=($BLOCKSIZE){ ... } ; powtórz N razy (jeden argument = licznik)
LOOP(R8){ ... }          ; powtórz R8 razy
```

Wewnątrz zmienna `I` to bieżąca wartość licznika. Klasyczne „wyślij 8 bitów":

```hpl
LOOP=(7,0){SI=R0[I],SCK=P}   ; od bitu 7 do 0: ustaw SI z bitu, takt zegara
```

Pętle można **zagnieżdżać**:

```hpl
LOOP=(0,7){ P1=0, LOOP=(6,1){P1=0,P1=1} }
```

### Warunki — operator `?`

```hpl
R1?0{ ... }        ; jeśli R1 == 0
R1?!0x30{ ... }    ; jeśli R1 != 0x30
R1?<200{ ... }     ; jeśli R1 < 200
R1?>99{ ... }      ; jeśli R1 > 99
PI1?1{ ... }       ; jeśli pin PI1 == 1
R9[0]?0{BREAK}     ; jeśli bit0 == 0 → przerwij pętlę
```

### Przerwania i pauzy

| Zapis | Działanie |
|-------|-----------|
| `BREAK` | przerwij najbliższą pętlę |
| `EXIT` | zakończ całą operację |
| `P=n` | pauza (opóźnienie), np. `P=25000` po zapisie |

Przykład oczekiwania na gotowość (odczyt bitu WIP):

```hpl
LOOP=(0,2000){
  ; ... odczytaj rejestr statusu do R9 ...
  R9[0]?0{BREAK}   ; bit gotowości = 0 → koniec czekania
  P=10
}
```

---

## 8. Podprogramy

Sekcja o nazwie zaczynającej się od `_` jest **podprogramem** — wywołujesz ją nazwą. Argument (jeśli podany) trafia zwykle do `R0`.

Definicja i wywołanie (z `25c256`):

```hpl
[_SEND]
LOOP=(7,0){SI=R0[I],SCK=P}     ; wyślij 8 bitów z R0

; ...gdzie indziej:
_SEND(00000011b)               ; wywołanie: R0=0x03, potem uruchom [_SEND]
```

Podprogramy bez argumentu (z `24c02`):

```hpl
[_START]
SDA=1,SCL=1,SDA=0,SCL=0

; użycie:
_START
```

To pozwala nie powtarzać w kółko sekwencji Start/Stop/„wyślij bajt".

---

## 9. Komunikacja z użytkownikiem

### `PRINT` — okna i logi

| Zapis | Efekt |
|-------|-------|
| `PRINT=("tekst")` | zwykłe okno |
| `PRINT=I("tekst")` | okno informacyjne |
| `PRINT=E("tekst")` | okno błędu |
| `PRINT=A("pytanie")` | okno OK/Anuluj → wynik w `RA` (1/0) |
| `PRINT=S("...")` | pasek statusu (nie blokuje) |
| `PRINT=L("...")` | wpis do logu |

Format jak `printf`: `%lu` (dziesiętnie), `%lX`/`%08lX` (hex, z wypełnieniem zerami), `\n` (nowa linia).

```hpl
R0=0x12345
PRINT=("R0=%08lXH",R0)          ; → R0=00012345H
PRINT=A("Kontynuować?")
RA?0{EXIT}                       ; Anuluj → wyjdź
```

### `GET` — wpisanie wartości

```hpl
GET=("Podaj kod:",R0,R4)         ; okno z polami dla R0 i R4
RA?1{PRINT=("R0=%04lXH",R0)}
```

---

## 10. Standardowe sekcje operacji

Orange5 wywołuje określone sekcje w odpowiedzi na przyciski GUI:

| Sekcja | Kiedy uruchamiana |
|--------|-------------------|
| `[!SETUP]` / `[!#SETUP]` | raz, przy wyborze chipu (inicjalizacja rejestrów, makr) |
| `[INIT]` | przed każdą operacją (ustawienie pinów w stan startowy) |
| `[READ]` | odczyt pojedynczej komórki (`ADR` → `DATA`) |
| `[READBLOCK]` | odczyt bloku (`$BLOCKSIZE` komórek) — szybciej |
| `[WRITEINIT]` / `[WRITE]` | zapis pojedynczej komórki (`DATA` pod `ADR`) |
| `[WRITEBLOCK]` | zapis bloku |
| `[ERASE]` | kasowanie |
| `[END]` | po operacji (bezpieczne stany pinów) |

Własne sekcje (np. `[ReadStatus]`, `[WriteStatus]`) pojawiają się jako dodatkowe funkcje skryptu.

---

## 11. Studium przypadku 1 — I2C 24Cxx

Rozbierzmy fragmenty **`24c02.hpl`** (EEPROM I2C 24C01/24C02). To modelowy przykład bit-bang I2C.

**Nagłówek i piny:**

```hpl
SOCKET=1 ;"I2C"

PING=SCL,0        ; zegar (dwukierunkowy/open-drain)
PING=SDA,1        ; dane (dwukierunkowy)
PINO=WP, 2        ; write-protect
PINO=A0, 3        ; adresowe piny układu
PINO=A1, 4
PINO=A2, 5

CDELAY = 4        ; szybkość zegara

R10=I2CADR,H2     ; alias: R10 = "I2CADR"

[!#SETUP]
R10=0xA0          ; bazowy adres I2C (1010_000x)
$WDELAY=25000     ; czas na zapis
```

**Podprogramy Start/Stop** — fundament I2C:

```hpl
[_START]
SDA=1,SCL=1,SDA=0,SCL=0          ; opadające SDA przy wysokim SCL = Start
SDA?1{                           ; kontrola: SDA powinno dać się ściągnąć
  P=200
  SDA?1{PRINT=E("SDA line error 1"),EXIT}
}

[_STOP]
SCL=0,SDA=0,SCL=1,SDA=1          ; narastające SDA przy wysokim SCL = Stop
```

**Odczyt bajtu (`[READ]`)** — pełen cykl I2C:

```hpl
[READ]
_START

LOOP=(7,0){SDA=R10[I],SCL=P}     ; wyślij bajt kontrolny 0xA0 (zapis adresu)
SDA=0,SDA=1,SCL=1,SDA?0          ; sprawdź ACK (SDA powinno być 0)
SCL=0,SDA=0

LOOP=(7,0){SDA=ADR[I],SCL=P}     ; wyślij adres komórki
SDA=0,SDA=1,SCL=1,SDA?0          ; ACK
SCL=0,SDA=0

_START                           ; powtórzony Start
LOOP=(7,0){SDA=R11[I],SCL=P}     ; bajt kontrolny 0xA1 (odczyt)
SDA=0,SDA=1,SCL=1,SDA?0          ; ACK
SCL=0

SDA=1                            ; zwolnij SDA (Hi-Z), teraz czyta chip
LOOP=(7,0){SCL=1,DATA[I]=SDA,SCL=0}   ; wczytaj 8 bitów do DATA
SDA=1,SCL=1,SCL=0,SDA=0          ; NACK (master nie prosi o więcej)
SCL=0,SCL=1,SDA=1                ; Stop
```

Zwróć uwagę na wzorzec **„ACK check"**: po każdym wysłanym bajcie master zwalnia SDA i sprawdza, czy chip ściągnął linię do 0 (`SDA?0`).

**Zapis (`[WRITE]`)** kończy się pauzą na wewnętrzny cykl zapisu:

```hpl
[WRITE]
_START
LOOP=(7,0){SDA=R10[I],SCL=P}     ; 0xA0
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
LOOP=(7,0){SDA=ADR[I],SCL=P}     ; adres
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
LOOP=(7,0){SDA=DATA[I],SCL=P}    ; dane
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
SCL=0,SCL=1,SDA=1                ; Stop
P=$WDELAY                        ; czekaj na zapis (25 ms)
```

**Odczyt blokowy** używa `$BLOCKSIZE` i wysyła ACK po każdym bajcie oprócz ostatniego (NACK):

```hpl
R8=$BLOCKSIZE
R8=-1
LOOP(R8){                        ; wszystkie prócz ostatniego
  LOOP=(7,0){SCL=1,DATA[I]=SDA,SCL=0}
  SDA=0, SCL=P, SDA=1            ; ACK
  ADR=+1
}
LOOP=(7,0){SCL=1,DATA[I]=SDA,SCL=0}  ; ostatni bajt
SDA=1, SCL=P                     ; NACK
_STOP
```

---

## 12. Studium przypadku 2 — Microwire 93Cxx

**`9306.hpl`** (NMC9306, 16×16, Microwire) pokazuje inny protokół: 3 linie + instrukcje.

**Piny i tryb:**

```hpl
SOCKET=2 ;"MW"
PINO=CLK,0
PINO=DI,1        ; wejście danych układu (my nadajemy)
PINO=CS,2        ; chip select
PINI=DO,1        ; wyjście danych układu (my czytamy)
CDELAY=8
```

**Odczyt** — instrukcja READ = start bit `1` + opcode `10` + adres:

```hpl
[READ]
CS=0
CLK=0
DI=0
CS=1
R0=011000B                        ; 0 1 1 0 0 0 = startbit(1)+op(10)+... (patrz nota)
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}  ; wyślij instrukcję (6 bitów)
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0} ; wyślij adres (4 bity)
DI=1
LOOP=(15,0){CLK=1,CLK=0,DATA[I]=DO}  ; wczytaj słowo 16-bit z DO
CS=0
```

**Zapis** wymaga najpierw odblokowania (`EWEN`), potem `ERASE`, `WRITE`, a na końcu blokady (`EWDS`):

```hpl
[WRITE]
CS=0
CLK=0
DI=0
CS=1
R0=0100110000B                    ; EWEN — Erase/Write Enable
LOOP=(9,0){DI=R0[I],CLK=1,CLK=0}

CS=0
DI=0
CLK=0
CS=1
R0=011100B                        ; ERASE + adres
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0}
CS=0
P=20000                           ; czekaj na kasowanie

DI=0
CLK=0
CS=1
R0=010100B                        ; WRITE + adres + dane
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0}
LOOP=(15,0){CLK=0,DI=DATA[I],CLK=1}  ; 16 bitów danych
CS=0
P=20000

[WRITEEND]
CLK=0
DI=0
CS=1
R0=0100000000B                    ; EWDS — Erase/Write Disable
LOOP=(9,0){DI=R0[I],CLK=1,CLK=0}
CS=0
```

Wniosek: w Microwire „instrukcje" to po prostu ciągi bitów, które wysyłasz pętlą `LOOP` na linii `DI`, taktując `CLK`.

---

## 13. Studium przypadku 3 — SPI 25Cxx

**`25c256.hpl`** (SPI EEPROM 16-bit adres). Wzorzec czterech linii: SCK, SI (MOSI), SO (MISO), CS.

```hpl
SOCKET=4 ;"SPI"
PINO=SCK,0
PINO=SI,1
PINO=CS,2
PINO=WP,3
PINO=HOLD,4
PINI=SO,1
CDELAY=1

[!SETUP]
$WDELAY=10000

[_SEND]                            ; wyślij bajt z R0 (MSB first)
LOOP=(7,0){SI=R0[I],SCK=P}

[READ]
CS=1
SCK=0
CS=0
_SEND(00000011b)                   ; komenda READ (0x03)
LOOP=(15,0){SI=ADR[I],SCK=P}       ; 16-bit adres
SI=1
R0=0
LOOP=(7,0){SCK=1,R0[I]=SO,SCK=0}   ; wczytaj bajt z SO
DATA=R0
CS=1

[WRITE]
SCK=0
CS=0,SI=0
_SEND(00000110b)                   ; WREN (write enable, 0x06)
SI=1,CS=1
P=20
CS=0,SI=0
_SEND(00000010b)                   ; WRITE (0x02)
LOOP=(15,0){SI=ADR[I],SCK=P}       ; adres
LOOP=(7,0){SI=DATA[I],SCK=P}       ; dane
SI=1,CS=1
P=$WDELAY                          ; czekaj na zapis
```

Porównaj trzy protokoły:

| | Linie | Kolejność |
|--|-------|-----------|
| **I2C** | SDA, SCL (2, open-drain) | Start → adr+RW → ACK → dane → ACK → Stop |
| **Microwire** | CLK, DI, DO, CS (4) | CS↑ → startbit+opcode+adr → dane |
| **SPI** | SCK, SI, SO, CS (4) | CS↓ → komenda → adr → dane → CS↑ |

We wszystkich przypadkach rdzeń jest ten sam: **pętla `LOOP` wysyłająca/odbierająca bity z taktowaniem zegara**.

---

## 14. Rejestracja i dystrybucja skryptów

Aby Twój skrypt pojawił się w menu Orange5, dodaj wpis do dowolnego pliku `*.cfg` (Orange5 skanuje wszystkie):

```
GROUP=MojeSkrypty
CHIP=My Chip,256,my_script.hpl
```

- `GROUP=` — nazwa grupy w menu.
- `CHIP=<nazwa>,<rozmiar>,<plik>` — pozycja menu. Rozmiar np. `256`, `1K`, `64x16`, `32K`.
- Plik `.hpl` umieść w katalogu `HPL\`.

**Dystrybucja znajomym:** wystarczy podać plik `.hpl` + linię `CHIP=`. Szyfrowanie `.hpx` **nie jest wymagane** — jawne `.hpl` działają tak samo i mieszają się z `.hpx` w konfiguracjach. Bezpieczny wzorzec: własny plik `Z-MojeSkrypty.cfg` (ładuje się na końcu, nie rusza plików producenta).

---

## 15. Dobre praktyki i debugowanie

- **Zaczynaj od `[INIT]`** ustawiającego wszystkie piny w bezpieczny stan.
- **Sprawdzaj ACK/odpowiedź** chipu (`SDA?0`, odczyt bitu gotowości) i pokazuj błąd przez `PRINT=E(...)`.
- **`CDELAY`** reguluje szybkość — jeśli chip nie odpowiada, zwiększ opóźnienie.
- **`P=$WDELAY`** po zapisie — pamięci potrzebują czasu na wewnętrzny cykl.
- **Komentuj** każdy blok (Start, ACK, adres, dane) — jak w oryginalnych skryptach.
- **Testuj logikę** w symulatorze **HPL Studio** (krokowe wykonanie, podgląd rejestrów i pinów) zanim dotkniesz sprzętu.
- **`hpltest.hpl`** w katalogu HPL to żywa dokumentacja — zawiera przykłady wszystkich słów kluczowych.

---

## 16. Ściąga (dodatek)

```
; --- nagłówek ---
INFO="..."            SOCKET=n            CDELAY=n
VCC=mV                BAUD=n

; --- piny ---
PINO=NAME,n           PINI=NAME,n         PING=NAME,n
PINO=#NAME,n          NAME=1 / =0 / =P    NAME=R0[i]

; --- rejestry ---
R0..RF  DATA  ADR  RA  I        R0[i]  DATA[i]  ADR[i]
R9=NAZWA,format,bity...          $MAKRO

; --- liczby ---
0x1F     1FH     1010B     31

; --- matematyka (w miejscu na lewym rejestrze) ---
=+  =-  =&  =|  =^  =<<  =>>  =*  =/  =%     (stała lub Rx)

; --- sterowanie ---
LOOP=(a,b){ }   LOOP=(N){ }   LOOP(Rx){ }
X?v{ }  X?!v{ }  X?<v{ }  X?>v{ }
BREAK   EXIT   P=n

; --- podprogramy ---
[_NAME]  ...            _NAME        _NAME(arg->R0)

; --- dialogi ---
PRINT=("..")  I(..) E(..) A(..) S(..) L(..)
GET=("..",Rx)          %lu %08lX \n         RA (1/0)

; --- sekcje operacji ---
[!SETUP] [INIT] [READ] [READBLOCK] [WRITE] [WRITEBLOCK] [ERASE] [END]

; --- rejestracja (.cfg) ---
GROUP=Nazwa
CHIP=Nazwa,Rozmiar,plik.hpl
```

---

*Ten przewodnik powstał na bazie autentycznych skryptów `24c02.hpl`, `9306.hpl`, `25c256.hpl`, `hpltest.hpl` i innych z dystrybucji Orange5. Uwagi i uzupełnienia mile widziane — kolejnym krokiem jest wersja angielska.*
