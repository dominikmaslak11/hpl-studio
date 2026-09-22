# HPL Studio

**Nowoczesne, przyjazne początkującym środowisko do tworzenia skryptów HPL dla programatora Orange5** — plus przewodnik po języku HPL oparty na prawdziwych skryptach.

> To niezależny, otwarty projekt narzędzi i dokumentacji. **Nie jest** powiązany z producentem Orange5 i **nie zawiera** żadnych zastrzeżonych plików Orange5 (`orange.exe`, `AJunk.dll`, plików `.hpx/.hhh`, tokenów `DATA\SSS` ani skryptów producenta). Zawiera wyłącznie oryginalną pracę autora.

---

## Co jest w repo

| Ścieżka | Opis |
|---------|------|
| `hpl-studio.html` | **HPL Studio** — samodzielny edytor HPL (jeden plik HTML). Kolorowanie składni, autouzupełnianie, krokowy symulator/debugger logiki, walidator składni, interaktywny samouczek, motywy jasny/ciemny/wysoki kontrast, generator wpisu `.cfg`, eksport `.hpl`. |
| `docs/Przewodnik_HPL_PL.md` | **Przewodnik po języku HPL** (PL) — 16 rozdziałów z rozbiorem prawdziwych skryptów I2C / Microwire / SPI. |
| `examples/helloworld.hpl` | Minimalny skrypt „Hello World" (okno dialogowe). |
| `examples/Z-MyScripts.cfg` | Przykład rejestracji skryptu w menu Orange5. |
| `tools/x64dbg-mcp-bridge/plugin.cpp` | Poprawka mostu MCP dla x64dbg (naprawiony `attach` + komenda `waitpaused`). |

## HPL Studio — jak używać

`hpl-studio.html` to jeden plik — otwórz go w przeglądarce (lub opublikuj jako artefakt). Funkcje:

- **Edytor** z podświetlaniem składni HPL i numeracją linii.
- **Podpowiedzi** (autocomplete) słów kluczowych, rejestrów, sekcji.
- **Debugger/symulator** — krokowe wykonanie logiki skryptu z podglądem rejestrów `R0–RF`, `ADR`, `DATA`, `I` i stanu pinów oraz konsolą `PRINT`. (Symuluje logikę, nie timing sprzętu.)
- **Walidator** — sprawdza nawiasy, cudzysłowy, strukturę sekcji.
- **Samouczek** — 8 interaktywnych lekcji z przykładami do wstawienia.
- **Szablony** — Hello World, I2C 24Cxx, SPI 25Cxx, Microwire 93Cxx.
- **Generator `.cfg`** i eksport `.hpl`.

## Nauka HPL

Zacznij od `docs/Przewodnik_HPL_PL.md`. Rozdziały prowadzą od podstaw (piny, rejestry, pętle) do trzech pełnych studiów przypadków opartych na autentycznych skryptach.

## Roadmap

- [x] HPL Studio: edytor, kolorowanie, autocomplete, debugger, walidator, samouczek, motywy
- [x] Przewodnik HPL (PL)
- [x] **A** — Asystent AI (Claude) wbudowany w HPL Studio
- [ ] **B** — Tryb graficzny „klocki" (Scratch/FBD) jako opcja
- [ ] **C** — Wersja EN przewodnika
- [ ] **D** — Wersja lokalna (Electron): zapis plików, integracja DeepSeek, most do debuggera

## Licencja

MIT (patrz `LICENSE`) — dotyczy oryginalnej pracy w tym repozytorium.

Uwaga: `tools/x64dbg-mcp-bridge/plugin.cpp` korzysta z pluginowego SDK x64dbg; x64dbg jest na licencji GPLv3. Plik jest tu jako łatka referencyjna.
