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

---

## Roadmap — ZREALIZOWANE

- [x] **A** — Asystent AI (Claude) w HPL Studio przez możliwość `sample` (web, bez klucza). Wybór modelu Claude/DeepSeek; DeepSeek w wersji lokalnej.
- [x] **B** — Tryb graficzny „klocki" (Scratch/FBD) jako opcja (modal generujący HPL).
- [x] **C** — Angielska wersja przewodnika (`docs/Guide_HPL_EN.md`).
- [x] **D** — Wersja lokalna (Electron) w `desktop/`: natywny zapis plików + AI DeepSeek/Claude po kluczu API.

## Repozytorium
GitHub (publiczne): https://github.com/dominikmaslak11/hpl-studio

## Pozostaje na przyszłość
- Most do debuggera w wersji desktop (sterowanie x64dbg z aplikacji).
- Powrót do RE szyfru HPX: pokonanie „die-on-break" AJunk (TitanHide kernel — `sc start TitanHide`) i przechwycenie procedury deszyfracji.


## Uwaga prawna
Repozytorium zawiera wyłącznie oryginalną pracę autora. Nie zawiera zastrzeżonych plików Orange5. Inżynieria wsteczna prowadzona dla interoperacyjności na własnym, legalnie posiadanym sprzęcie/oprogramowaniu.
