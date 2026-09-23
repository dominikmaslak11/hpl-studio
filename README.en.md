# HPL Studio

**A modern, beginner-friendly environment for writing HPL scripts for the Orange5 programmer** — plus an HPL language guide built from real scripts.

*Języki / Languages:* **English** · [Polski](README.md)

> An independent, open tools-and-docs project. **Not** affiliated with the Orange5 vendor and **does not** contain any proprietary Orange5 files (`orange.exe`, `AJunk.dll`, `.hpx/.hhh` files, `DATA\SSS` tokens, or vendor scripts). It contains only the author's original work.

---

## What's in the repo

| Path | Description |
|------|-------------|
| `hpl-studio.html` | **HPL Studio** — a standalone HPL editor (single HTML file). Syntax highlighting, autocomplete, a step simulator/logic debugger, a syntax validator, an interactive tutorial, light/dark/high-contrast themes, PL/EN language switch, a `.cfg` entry generator, open/save `.hpl`, and a built-in AI assistant. |
| `docs/Guide_HPL_EN.md` | **HPL language guide** (EN) — 16 chapters dissecting real I2C / Microwire / SPI scripts. |
| `docs/Przewodnik_HPL_PL.md` | Polish edition of the guide. |
| `examples/helloworld.hpl` | Minimal "Hello World" script (dialog box). |
| `examples/Z-MyScripts.cfg` | Example of registering a script in the Orange5 menu. |
| `tools/x64dbg-mcp-bridge/plugin.cpp` | A fix for the x64dbg MCP bridge (fixed `attach` + a `waitpaused` command). |
| `desktop/` | Desktop build (Electron): native file save + AI (DeepSeek/Claude via API keys). |

## HPL Studio — how to use

`hpl-studio.html` is a single file — open it in a browser (or publish it as an artifact). Features:

- **Editor** with HPL syntax highlighting and line numbers.
- **Autocomplete** for keywords, registers and sections.
- **Debugger/simulator** — step through the script logic with a live view of registers `R0–RF`, `ADR`, `DATA`, `I`, pin states and a `PRINT` console. (Simulates logic, not hardware timing.)
- **Validator** — checks brackets, quotes and section structure.
- **Tutorial** — 8 interactive lessons with insertable examples.
- **Templates** — Hello World, I2C 24Cxx, SPI 25Cxx, Microwire 93Cxx.
- **Themes** — light / dark / high-contrast / system, and a **Polish/English** switch.
- **Files** — open an existing `.hpl` and save/download your work.
- **AI assistant** — Claude natively on the web (no key); Claude + DeepSeek via API keys in the desktop build.
- **Blocks** — an optional Scratch/FBD-style block builder that generates HPL.

## Learning HPL

Start with `docs/Guide_HPL_EN.md`. The chapters go from basics (pins, registers, loops) to three full case studies based on authentic scripts.

## Desktop build

See `desktop/README.md`. Runs HPL Studio outside the browser sandbox, adding native file save and AI via API keys (DeepSeek, Claude/Anthropic).

## Roadmap

- [x] HPL Studio: editor, highlighting, autocomplete, debugger, validator, tutorial, themes
- [x] HPL guide (PL + EN)
- [x] **A** — Built-in AI assistant (Claude)
- [x] **B** — Optional Scratch/FBD block mode
- [x] **C** — English guide
- [x] **D** — Desktop (Electron) build: native file save + DeepSeek/Claude via API key
- [x] PL/EN language switch, open files from disk
- [ ] Debugger bridge in the desktop build

- [x] Semantic linter (unknown subroutine, write to input pin, undefined macro, bit index, unknown pin)
- [x] Virtual 24C02 (I2C) chip in the debugger — a test bench that verifies read/write logically
- [x] Waveform view (SDA/SCK/…) and virtual **SPI 25xx** and **Microwire 93C46** test benches (alongside 24C02)
- [x] Hex editor + editable virtual-chip memory (load a dump, edit bytes, test the script against your own dump)
- [x] Chip recognition from a dump (by size/patterns → 24Cxx/93Cxx/25xx candidates)
- [x] Write-script generator (dump/changed bytes → HPL 'patch' for 24C02/25xx/93C46)
- [x] AI dump analysis (hex slice + context → fields/odometer/checksum analysis)
- [x] Dump analysis: diff two dumps (A vs B) + known-value locator (LE/BE/BCD/complement/scales) with offset jump
## License

MIT (see `LICENSE`) — covers the original work in this repository.

Note: `tools/x64dbg-mcp-bridge/plugin.cpp` uses the x64dbg plugin SDK; x64dbg is GPLv3-licensed. The file is included here as a reference patch.
