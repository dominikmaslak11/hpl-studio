// Bezpieczny most renderer → proces glowny.
// Udostepnia window.hplNative, ktore HPL Studio wykrywa i uzywa zamiast trybu web.
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('hplNative', {
  ai:          (opts) => ipcRenderer.invoke('ai-complete', opts),   // {provider, prompt} -> tekst
  saveFile:    (opts) => ipcRenderer.invoke('save-file', opts),     // {defaultName, data} -> sciezka | null
  openFile:    ()     => ipcRenderer.invoke('open-file'),           // -> {path, data} | null
  getSettings: ()     => ipcRenderer.invoke('get-settings'),
  setSettings: (s)    => ipcRenderer.invoke('set-settings', s)
});
