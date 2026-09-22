// HPL Studio — proces glowny Electron
// Uruchamia HPL Studio poza sandboxem przegladarki i dostarcza natywne moznosci:
//  - zapis/otwarcie pliku .hpl
//  - asystent AI przez klucze API (DeepSeek, Claude/Anthropic)
// Klucze zapisywane sa lokalnie w katalogu danych aplikacji (nigdzie nie wysylane
// poza wybrane API).

const { app, BrowserWindow, ipcMain, dialog, Menu, shell } = require('electron');
const path = require('path');
const fs = require('fs');

const settingsPath = () => path.join(app.getPath('userData'), 'hpl-studio-settings.json');
function readSettings() {
  try { return JSON.parse(fs.readFileSync(settingsPath(), 'utf8')); } catch (_) { return {}; }
}
function writeSettings(s) {
  try { fs.writeFileSync(settingsPath(), JSON.stringify(s, null, 2)); return true; } catch (_) { return false; }
}

let win;

function createWindow() {
  win = new BrowserWindow({
    width: 1320, height: 860,
    backgroundColor: '#16130e',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  win.loadFile(path.join(__dirname, '..', 'hpl-studio.html'));
  buildMenu();
}

function openSettings() {
  const w = new BrowserWindow({
    width: 540, height: 460, parent: win, modal: true, resizable: false,
    backgroundColor: '#1e1a14',
    webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true }
  });
  w.setMenuBarVisibility(false);
  w.loadFile(path.join(__dirname, 'settings.html'));
}

function buildMenu() {
  const menu = Menu.buildFromTemplate([
    { label: 'Plik', submenu: [
      { label: 'Ustawienia → Klucze API…', accelerator: 'CmdOrCtrl+,', click: openSettings },
      { type: 'separator' },
      { role: 'quit', label: 'Zamknij' }
    ]},
    { label: 'Widok', submenu: [
      { role: 'reload', label: 'Odswiez' },
      { role: 'toggleDevTools', label: 'Narzedzia deweloperskie' },
      { type: 'separator' },
      { role: 'resetZoom', label: 'Zoom 100%' }, { role: 'zoomIn' }, { role: 'zoomOut' },
      { type: 'separator' }, { role: 'togglefullscreen', label: 'Pelny ekran' }
    ]},
    { label: 'Pomoc', submenu: [
      { label: 'Repozytorium GitHub', click: () => shell.openExternal('https://github.com/dominikmaslak11/hpl-studio') }
    ]}
  ]);
  Menu.setApplicationMenu(menu);
}

// ── IPC: pliki ───────────────────────────────────────────────
ipcMain.handle('save-file', async (_e, { defaultName, data }) => {
  const r = await dialog.showSaveDialog(win, {
    defaultPath: defaultName || 'script.hpl',
    filters: [{ name: 'Skrypt HPL', extensions: ['hpl'] }, { name: 'Wszystkie pliki', extensions: ['*'] }]
  });
  if (r.canceled || !r.filePath) return null;
  fs.writeFileSync(r.filePath, data, 'utf8');
  return r.filePath;
});

ipcMain.handle('open-file', async () => {
  const r = await dialog.showOpenDialog(win, {
    properties: ['openFile'],
    filters: [{ name: 'HPL / tekst', extensions: ['hpl', 'txt', 'cfg'] }, { name: 'Wszystkie', extensions: ['*'] }]
  });
  if (r.canceled || !r.filePaths[0]) return null;
  return { path: r.filePaths[0], data: fs.readFileSync(r.filePaths[0], 'utf8') };
});

// ── IPC: ustawienia ──────────────────────────────────────────
ipcMain.handle('get-settings', () => readSettings());
ipcMain.handle('set-settings', (_e, s) => { const merged = { ...readSettings(), ...s }; writeSettings(merged); return merged; });

// ── IPC: AI (DeepSeek / Claude) ──────────────────────────────
ipcMain.handle('ai-complete', async (_e, { provider, prompt }) => {
  const s = readSettings();

  if (provider === 'deepseek') {
    const key = s.deepseekKey;
    if (!key) throw new Error('Brak klucza API DeepSeek — ustaw go w menu Plik → Ustawienia.');
    const base = (s.deepseekBase || 'https://api.deepseek.com').replace(/\/$/, '');
    const res = await fetch(base + '/chat/completions', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + key },
      body: JSON.stringify({ model: s.deepseekModel || 'deepseek-chat', messages: [{ role: 'user', content: prompt }], stream: false })
    });
    if (!res.ok) throw new Error('DeepSeek HTTP ' + res.status + ': ' + (await res.text()).slice(0, 300));
    const j = await res.json();
    return (j.choices && j.choices[0] && j.choices[0].message && j.choices[0].message.content) || '';
  }

  if (provider === 'claude') {
    const key = s.claudeKey;
    if (!key) throw new Error('Brak klucza API Claude — ustaw go w menu Plik → Ustawienia.');
    const res = await fetch('https://api.anthropic.com/v1/messages', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'x-api-key': key, 'anthropic-version': '2023-06-01' },
      body: JSON.stringify({ model: s.claudeModel || 'claude-sonnet-5', max_tokens: 2048, messages: [{ role: 'user', content: prompt }] })
    });
    if (!res.ok) throw new Error('Claude HTTP ' + res.status + ': ' + (await res.text()).slice(0, 300));
    const j = await res.json();
    return (j.content || []).map(b => b.text || '').join('') || '';
  }

  throw new Error('Nieznany dostawca AI: ' + provider);
});

app.whenReady().then(createWindow);
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0) createWindow(); });
