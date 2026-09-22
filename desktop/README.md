# HPL Studio — wersja desktop (Electron)

Uruchamia **HPL Studio** lokalnie, poza sandboxem przeglądarki. Dzięki temu działa to, czego wersja webowa nie może:

- **Prawdziwy zapis pliku** `.hpl` (natywne okno zapisu) — przycisk „Pobierz .hpl".
- **Asystent AI przez klucze API**: wybierz w panelu AI **DeepSeek** lub **Claude** — zapytania idą do wybranego API z Twoim kluczem.

To ten sam plik `../hpl-studio.html` — wykrywa `window.hplNative` i przełącza się w tryb natywny (zapis + AI po kluczach). W przeglądarce ten sam plik używa natywnego Claude (bez klucza).

## Wymagania
- Node.js 18+ (zawiera `npm`).

## Uruchomienie
```bash
cd desktop
npm install     # pobiera Electron
npm start
```

## Klucze API
Menu **Plik → Ustawienia → Klucze API** (Ctrl+,):
- **DeepSeek** — klucz z `platform.deepseek.com` (zakładka API keys). DeepSeek używa wyłącznie klucza API (brak logowania przez przeglądarkę).
- **Claude** — klucz z `console.anthropic.com`.

Klucze zapisują się w pliku ustawień w katalogu danych aplikacji (`app.getPath('userData')`) i nie są wysyłane nigdzie poza wybrane API.

## Budowanie instalatora (opcjonalnie)
Dodaj `electron-builder` jako devDependency i skonfiguruj sekcję `build` w `package.json`, np.:
```bash
npm i -D electron-builder
npx electron-builder --win
```

## Uwagi
- Model dla Claude/DeepSeek jest konfigurowalny w Ustawieniach — dostosuj identyfikator do aktualnie dostępnych modeli danego API.
- Strumieniowanie odpowiedzi (token po tokenie) w wersji desktop jest uproszczone (odpowiedź pojawia się w całości); można je dodać przez zdarzenia IPC.
