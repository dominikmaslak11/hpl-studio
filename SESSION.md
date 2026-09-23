# Log sesji — projekt HPL / Orange5

Zapis prac nad narzędziami i dokumentacją HPL dla Orange5 oraz analizą formatu HPX.

---

## Cele

1. Stworzyć skrypt „Hello World" w HPL i móc tworzyć/rozpowszechniać własne skrypty.
2. Zrozumieć i (docelowo) odtworzyć szyfrowanie plików `.hpx`.
3. Zbudować przyjazne środowisko (IDE) do pisania skryptów HPL.
4. Napisać przewodnik po HPL na bazie prawdziwych skryptów.

---

## Co zostało zrobione

### 1. Hello World + podstawy HPL
- `examples/helloworld.hpl` — pokazuje okno `PRINT=("Hello World!")`.
- `examples/Z-MyScripts.cfg` — rejestracja w menu (Orange5 skanuje wszystkie `*.cfg`).
- Ustalono: **skrypty `.hpl` są jawnym tekstem i działają bez szyfrowania**; mieszają się z `.hpx` w configach. Do dystrybucji szyfrowanie nie jest potrzebne.

### 2. Analiza formatu HPX (szyfrowanie)
- `.hpx` = 16 B jawnego nagłówka (`HPX\0`, rozmiar 0x7E=126, flagi) + zaszyfrowane ciało.
- `[16..125]` = 110 B zaszyfrowanego nagłówka per-plik (materiał klucza).
- Szyfrowanie jest **per-plik** (brak reużycia keystreamu: `C1⊕C2` ≈ szum) — brak skrótu offline.
- `orange.exe` **nie jest spakowany**, ma statycznie wlinkowany OpenSSL (stringi wycięte), **nie ma** wbudowanego eksportu do `.hpx`.
- Potwierdzono, że orange trzyma skrypty w pamięci jako **tekst HPL** (dump `comsvcs MiniDump`), ale `.hpx` **nie deszyfruje się** przy samym wyborze/nieudanym odczycie — deszyfracja wymaga realnej operacji lub przechwycenia debuggerem.
- **Wniosek:** pełne odtworzenie szyfru = duży projekt RE; szyfr żyje w zaciemnionym `AJunk.dll`.

### 3. Anti-debug Orange5 (AJunk)
- Dialog „A debugger has been found running…" — sprawdzenie debug-portu w DllMain przy starcie.
- **Obejście, które działa: attach-after** (uruchom orange normalnie, potem podłącz debugger).
- ScyllaHide (user-mode) nie ukrywa startowego sprawdzenia; przy attach rzuca `Distorm GetSysCallIndex32` (AJunk inline-hookuje ntdll).
- **Nowy mur:** AJunk **zabija proces przy każdym wejściu debuggera w przerwanie** (pause/breakpoint). Do pokonania potrzebny anti-anti-debug przeżywający break (TitanHide kernel — sterownik zainstalowany, system w trybie test-signing; wymaga ręcznego `sc start TitanHide`).

### 4. Naprawa mostu MCP dla x64dbg
- `tools/x64dbg-mcp-bridge/plugin.cpp` — bug: handler `exec` (którym idzie `attach`) nie czekał na pauzę → `readmem`/`getregs` padały.
- Poprawka: po `attach`/`init` most czeka (do 10 s) na `DbgIsDebugging() && !DbgIsRunning()`; dodano komendę `waitpaused`.
- Po naprawie `attach 0xPID` → `mod.base(orange.exe)=0x400000` **działa**.
- Uwaga budowania: toolchain MSYS2 mingw32 rozjechany (spawn `as`/`ld` przez gcc pada) — buduj ręcznie: `g++ -S` → `as --32` → `ld` z `--start-group`; dołóż `libstdc++-6.dll`, `libgcc_s_dw2-1.dll`, `libwinpthread-1.dll` do katalogu x32dbg.

### 5. HPL Studio (IDE) — `hpl-studio.html`
- Edytor z kolorowaniem składni + numeracją linii.
- Autouzupełnianie (słowa kluczowe, rejestry, sekcje).
- Krokowy symulator/debugger logiki (rejestry, piny, konsola PRINT).
- Walidator składni (nawiasy, cudzysłowy, sekcje).
- Interaktywny samouczek (8 lekcji).
- Motywy: jasny / ciemny / wysoki kontrast / system.
- Szablony (Hello World, I2C, SPI, Microwire), generator `.cfg`, eksport `.hpl`.
- Opublikowane jako artefakt (link prywatny właściciela).

### 6. Przewodnik HPL — `docs/Przewodnik_HPL_PL.md`
- 16 rozdziałów; studia przypadków I2C `24c02`, Microwire `9306`, SPI `25c256`.


### 7. HPL Studio — narzędzia inżynierii wstecznej dumpów (v9–v16)
- **Linter semantyczny** + **wirtualne układy** w debuggerze: 24C02 (I2C), 25xx (SPI), 93C46 (Microwire) z widokiem przebiegów (SDA/SCK/…). Test-benche sprawdzają logikę odczytu/zapisu na WŁASNYM dumpie.
- **Edytor hex (ala HxD)** + edytowalna pamięć wirtualnego układu (VMEM, Uint8Array, zapis w localStorage).
- **Rozpoznanie chipu z dumpu** (rozmiar/wzorce → kandydaci 24Cxx/93Cxx/25xx).
- **Generator skryptu zapisującego** (dump / zmienione bajty → HPL „patch”).
- **AI analiza dumpu** (wycinek hex + kontekst).
- **Diff dwóch dumpów** (A vs B) + **lokalizator znanej wartości** (LE/BE/BCD/dopełnienie/skale) z podświetleniem offsetu.
- **Kalkulator sumy kontrolnej**: SUM8/SUM16/XOR/CRC16 (CCITT-FALSE, MODBUS) nad zakresem; wektory testowe PASS.
- **Presety kodowań desek + dekoder wartości**: wybór presetu, offset/bajty/endian/skala/jednostka, Odczyt/Zapis skalowanej wartości bezpośrednio w VMEM.

### 8. RE licznika godzin 9680 (93C66) — ROZWIĄZANE
- Dane: `C:\hack\trainingData\9680.bin` i `9680_2.bin` (512 B, 93C66 256×16, word-organized).
- Oba dumpy różnią się **tylko** bajtami **0x150–0x151** (= word 0x0A8) → tam jest licznik.
- **Algorytm: raw = godziny × 20, little-endian** (1 count = 3 min); NIE szyfrowanie, skalowany licznik.
  - `9680.bin`: `d0 07 00` = 2000 → **100 h**
  - `9680_2.bin`: `d4 08 00` = 2260 → **113 h**
- Zapis: bajty = LE(godziny × 20) — 113 h → `d4 08 00`, 100 h → `d0 07 00`. Zweryfikowane headless (PASS).
- Preset „9680 dash — godziny silnika (93C66)” w HPL Studio ustawiony na skalę **×20**.
- Nauka: jeden punkt (raw↔odczyt) nie wystarcza do ustalenia skali (pierwotny błędny ×200 z odczytu „10 h”, faktycznie 100 h) — potrzebne były dwa dumpy.
---

## Roadmap — ZREALIZOWANE

- [x] **A** — Asystent AI (Claude) w HPL Studio przez możliwość `sample` (web, bez klucza). Wybór modelu Claude/DeepSeek; DeepSeek w wersji lokalnej.
- [x] **B** — Tryb graficzny „klocki" (Scratch/FBD) jako opcja (modal generujący HPL).
- [x] **C** — Angielska wersja przewodnika (`docs/Guide_HPL_EN.md`).
- [x] **D** — Wersja lokalna (Electron) w `desktop/`: natywny zapis plików + AI DeepSeek/Claude po kluczu API.

## Dodatkowo zrobione
- [x] **Dwujęzyczne IDE (PL/EN)** — pełny przełącznik języka (i18n: statyczne UI, ściąga, samouczek, bloki, walidator, komunikaty debuggera/AI); wybór zapisywany lokalnie.
- [x] **Otwieranie plików `.hpl`** z dysku (web: file input; desktop: natywne okno).
- [x] **Dwujęzyczne README** — `README.md` (PL) + `README.en.md` (EN) z odnośnikami.
- [x] **Motywy** jasny/ciemny/wysoki kontrast/system.
- [x] Materiały promocyjne: post na MHHAuto (kategoria „EEPROM – Microcontroller") oraz wiadomość do znajomego Vlada (`C:\hack\message_to_Vlads_friend.md`, poza repo).

Wersje artefaktu HPL Studio: v1→v16 (v8 = dwujęzyczność + otwieranie plików; v9–v14 = linter, wirtualne układy, hex, analiza dumpów; v15–v16 = presety/dekoder wartości, skala 9680 poprawiona na ×20). Skrót do repo w Claude Code: `/artifacts`.

## Ścieżki lokalne
- Repo: `C:\hack\hpl-studio\` · IDE: `C:\hack\hpl-studio\hpl-studio.html`
- Przewodniki: `docs\Przewodnik_HPL_PL.md`, `docs\Guide_HPL_EN.md` · Desktop: `desktop\`
- gh CLI: `C:\hack\tools\gh\bin\gh.exe` (zalogowany, keyring). Git credential helper ustawiony lokalnie w repo na gh.

## Repozytorium
GitHub (publiczne): https://github.com/dominikmaslak11/hpl-studio

## Pozostaje na przyszłość
- Most do debuggera w wersji desktop (sterowanie x64dbg z aplikacji).
- Powrót do RE szyfru HPX: pokonanie „die-on-break" AJunk (TitanHide kernel — `sc start TitanHide` jako admin) i przechwycenie procedury deszyfracji.
- Opcjonalnie: publiczne udostępnienie artefaktu (Share) dla klikalnego linku online; screenshoty do posta; wersja RU wiadomości.


## Uwaga prawna
Repozytorium zawiera wyłącznie oryginalną pracę autora. Nie zawiera zastrzeżonych plików Orange5. Inżynieria wsteczna prowadzona dla interoperacyjności na własnym, legalnie posiadanym sprzęcie/oprogramowaniu.
