# HPL Language Guide (Orange5)

*Learn the Orange5 programmer's HPL scripting language from real, plain-text `.hpl` scripts.*

> English edition. All examples come from authentic files in the `HPL\` folder of the Orange5 distribution. This is the international version of the Polish guide (`Przewodnik_HPL_PL.md`).

---

## Table of contents

1. [Introduction](#1-introduction)
2. [Anatomy of a script](#2-anatomy-of-a-script)
3. [Pins — PINO / PINI / PING](#3-pins)
4. [Registers and data](#4-registers-and-data)
5. [Numbers, constants and macros](#5-numbers-constants-and-macros)
6. [Operations and math](#6-operations-and-math)
7. [Control flow: loops and conditions](#7-control-flow)
8. [Subroutines](#8-subroutines)
9. [User interaction: PRINT and GET](#9-user-interaction)
10. [Standard operation sections](#10-standard-operation-sections)
11. [Case study 1 — I2C 24Cxx](#11-case-study-1--i2c-24cxx)
12. [Case study 2 — Microwire 93Cxx](#12-case-study-2--microwire-93cxx)
13. [Case study 3 — SPI 25Cxx](#13-case-study-3--spi-25cxx)
14. [Registering and distributing scripts](#14-registering-and-distributing-scripts)
15. [Best practices and debugging](#15-best-practices-and-debugging)
16. [Cheat sheet (appendix)](#16-cheat-sheet-appendix)

---

## 1. Introduction

**HPL** (Hardware Programming Language) is the scripting language of the **Orange5** device programmer. An HPL script describes **how to drive the socket pins** (bit by bit) to read or write a memory/microcontroller placed in the ZIF socket.

Key facts:

- An HPL script is a **plain text** `.hpl` file — open and edit it in any editor.
- The program *bit-bangs* the protocol: you set the clock and data lines yourself, send bits and read them back. HPL has no built-in "I2C"; you build the protocol from individual pin states.
- Files live in the `HPL\` folder of the Orange5 install. Encrypted `.hpx` counterparts are the same logic, merely hidden — **you do not need encryption to create or share your own scripts**.

Minimal skeleton:

```hpl
; a comment starts with a semicolon
INFO="Chip description"
SOCKET=0

[Hello]
PRINT=("Hello World!")

[END]
```

---

## 2. Anatomy of a script

A script has a **header** (global directives) and **sections** in square brackets `[NAME]`.

```hpl
; Orange programmer module v2.9
SOCKET=1 ;"I2C"          <- socket / mode type
PINO=WP, 2              <- pin definitions (header)
CDELAY = 4             <- global clock delay

[!#SETUP]              <- special: runs once when the chip is selected
R10=0xA0

[INIT]                 <- standard operation sections
[READ]
[WRITE]
[END]
```

| Element | Meaning |
|--------|---------|
| `; ...` | comment to end of line |
| `INFO="..."` | description shown in the GUI |
| `SOCKET=n` | socket/mode (0 plain, 1 I2C, 2 MW/Microwire, 4 SPI — per the scripts' comments) |
| `CDELAY=n` | how long one clock state lasts (speed) |
| `BAUD=n` | baud rate for serial modes (UART/K-Line) |
| `VCC=mV` | supply voltage in millivolts |
| `[NAME]` | section — an execution entry point. Standard operations have fixed names (ch. 10); custom names appear as script functions in the GUI |

Sections whose names start with `_` are **subroutines** (ch. 8); `[!SETUP]` / `[!#SETUP]` run at chip selection.

---

## 3. Pins

A pin definition maps a **name** to a socket **pin number**.

```hpl
PINO=SCK,0      ; output on pin 0, available as "SCK"
PINO=#RW,9      ; INVERTED output (# before the name)
PINI=SO,1       ; input on pin 1
PING=SDA,1      ; bidirectional (open-drain) pin, typical for I2C
```

| Directive | Role |
|-----------|------|
| `PINO=NAME,n` | **output** pin — you drive it |
| `PINI=NAME,n` | **input** pin — you read it |
| `PING=NAME,n` | **bidirectional** pin (e.g. I2C SDA/SCL, open-drain lines) |
| `PINO=#NAME,n` | **inverted** output (negative logic) |

Driving a pin:

```hpl
SCK=1          ; drive high
SCK=0          ; drive low
SCK=P          ; clock PULSE: does 0→1→0 (one "tick")
SDA=R0[3]      ; set pin from bit 3 of R0
DATA[I]=SDA    ; read pin into bit I of the data word
```

`=P` is the most common shortcut — one clock pulse. Separate multiple statements on a line with commas:

```hpl
SDA=1,SCL=1,SDA=0,SCL=0      ; I2C Start sequence
```

---

## 4. Registers and data

HPL provides 16 **32-bit** registers: `R0`–`RF` (i.e. R0..R9, RA..RF; hex index). Plus special variables:

| Name | Meaning |
|------|---------|
| `R0`–`RF` | user registers (32-bit) |
| `DATA` | byte/word of the current cell (what you read/write) |
| `ADR` | address of the current cell |
| `RA` | result of a `PRINT=A` box (1 = OK, 0 = Cancel) — note `RA` is also register R10 |
| `I` | `LOOP` counter |

**Bit access:** `R0[3]` is bit 3 of `R0` (0 = least significant). Works for `DATA[I]`, `ADR[I]` too.

```hpl
R0=0
R0[7]=1          ; set bit 7  → R0 = 0x80
DATA[6]=SDA      ; read pin SDA into bit 6 of the data word
```

**Register aliases** — you can name a register in the header:

```hpl
R10=I2CADR,H2    ; R10 named "I2CADR", display format: hex, 2 digits
R9=STATUS,C8,WPEN,x,x,x,BL1,BL0,WEL,RDY   ; name + bit names
```

The letter after the comma is the display format: `H` = hex, `D`/`d` = decimal, `B` = binary, a digit = number of positions.

---

## 5. Numbers, constants and macros

Three number notations — **all equivalent**:

```hpl
CONST=0x123B     ; hex with 0x prefix
CONST=123BH      ; hex with H suffix
CONST=1010B      ; binary with B suffix
R0=12345         ; decimal
```

**Macros** start with `$`, usually defined in `[!SETUP]`:

```hpl
[!#SETUP]
$WDELAY=25000     ; write wait time
```

Then use `$WDELAY` as a value: `P=$WDELAY`. Common environment-provided macro: `$BLOCKSIZE` (block size of the current operation).

`CONST=` also sends a constant value without a register.

---

## 6. Operations and math

Operations act **in place** on the left-hand register (`R0=+1` means `R0 = R0 + 1`):

| Syntax | Action |
|--------|--------|
| `R0=+1` `R0=-2` | add / subtract a constant |
| `R0=+R1` `R0=-R2` | add / subtract a register |
| `R0=&0x0FFF` | AND |
| `R0=\|1` | OR |
| `R0=^0xFF` | XOR |
| `R0=<<1` | shift left |
| `R0=>>2` | shift right |
| `R0=*R4` `R0=/R4` `R0=%R4` | multiply / divide / modulo |
| `R7=R2` | copy a register |

---

## 7. Control flow

### Loops — `LOOP`

Two forms:

```hpl
LOOP=(7,0){ ... }        ; I runs 7,6,...,0  (down)
LOOP=(0,15){ ... }       ; I runs 0,1,...,15 (up)
LOOP=($BLOCKSIZE){ ... } ; repeat N times (single argument = counter)
LOOP(R8){ ... }          ; repeat R8 times
```

Inside, `I` is the current counter value. The classic "send 8 bits":

```hpl
LOOP=(7,0){SI=R0[I],SCK=P}   ; bit 7 down to 0: set SI from bit, clock
```

Loops can be **nested**:

```hpl
LOOP=(0,7){ P1=0, LOOP=(6,1){P1=0,P1=1} }
```

### Conditions — the `?` operator

```hpl
R1?0{ ... }        ; if R1 == 0
R1?!0x30{ ... }    ; if R1 != 0x30
R1?<200{ ... }     ; if R1 < 200
R1?>99{ ... }      ; if R1 > 99
PI1?1{ ... }       ; if pin PI1 == 1
R9[0]?0{BREAK}     ; if bit0 == 0 → break the loop
```

### Breaks and pauses

| Syntax | Action |
|--------|--------|
| `BREAK` | break the nearest loop |
| `EXIT` | end the whole operation |
| `P=n` | pause (delay), e.g. `P=25000` after a write |

Wait-for-ready pattern (poll a WIP bit):

```hpl
LOOP=(0,2000){
  ; ... read status register into R9 ...
  R9[0]?0{BREAK}   ; ready bit = 0 → done
  P=10
}
```

---

## 8. Subroutines

A section whose name starts with `_` is a **subroutine** — you call it by name. An argument (if given) usually goes into `R0`.

```hpl
[_SEND]
LOOP=(7,0){SI=R0[I],SCK=P}     ; send 8 bits from R0

; elsewhere:
_SEND(00000011b)               ; call: R0=0x03, then run [_SEND]
```

Argument-less subroutines:

```hpl
[_START]
SDA=1,SCL=1,SDA=0,SCL=0

; use:
_START
```

---

## 9. User interaction

### `PRINT` — dialogs and logs

| Syntax | Effect |
|--------|--------|
| `PRINT=("text")` | plain box |
| `PRINT=I("text")` | info box |
| `PRINT=E("text")` | error box |
| `PRINT=A("question")` | OK/Cancel box → result in `RA` (1/0) |
| `PRINT=S("...")` | status line (non-blocking) |
| `PRINT=L("...")` | log entry |

`printf`-style format: `%lu` (decimal), `%lX`/`%08lX` (hex, zero-padded), `\n` (newline).

```hpl
R0=0x12345
PRINT=("R0=%08lXH",R0)          ; → R0=00012345H
PRINT=A("Continue?")
RA?0{EXIT}                       ; Cancel → exit
```

### `GET` — enter values

```hpl
GET=("Enter code:",R0,R4)        ; box with fields for R0 and R4
RA?1{PRINT=("R0=%04lXH",R0)}
```

---

## 10. Standard operation sections

Orange5 calls specific sections in response to GUI buttons:

| Section | When run |
|--------|----------|
| `[!SETUP]` / `[!#SETUP]` | once, when the chip is selected |
| `[INIT]` | before every operation (pins to a safe start state) |
| `[READ]` | read a single cell (`ADR` → `DATA`) |
| `[READBLOCK]` | read a block (`$BLOCKSIZE` cells) — faster |
| `[WRITEINIT]` / `[WRITE]` | write a single cell (`DATA` at `ADR`) |
| `[WRITEBLOCK]` | write a block |
| `[ERASE]` | erase |
| `[END]` | after the operation (safe pin states) |

Custom sections (e.g. `[ReadStatus]`) appear as extra script functions.

---

## 11. Case study 1 — I2C 24Cxx

Excerpts from **`24c02.hpl`** (I2C EEPROM 24C01/24C02) — a model bit-bang I2C example.

**Header and pins:**

```hpl
SOCKET=1 ;"I2C"
PING=SCL,0        ; clock (bidirectional/open-drain)
PING=SDA,1        ; data (bidirectional)
PINO=WP, 2        ; write-protect
PINO=A0, 3        ; device address pins
CDELAY = 4        ; clock speed
R10=I2CADR,H2     ; alias: R10 = "I2CADR"

[!#SETUP]
R10=0xA0          ; base I2C address (1010_000x)
$WDELAY=25000     ; write time
```

**Start/Stop subroutines** — the I2C foundation:

```hpl
[_START]
SDA=1,SCL=1,SDA=0,SCL=0          ; SDA falling while SCL high = Start
SDA?1{                           ; check: SDA should be pullable low
  P=200
  SDA?1{PRINT=E("SDA line error 1"),EXIT}
}

[_STOP]
SCL=0,SDA=0,SCL=1,SDA=1          ; SDA rising while SCL high = Stop
```

**Read a byte (`[READ]`)** — a full I2C cycle:

```hpl
[READ]
_START
LOOP=(7,0){SDA=R10[I],SCL=P}     ; send control byte 0xA0 (write address)
SDA=0,SDA=1,SCL=1,SDA?0          ; check ACK (SDA should be 0)
SCL=0,SDA=0
LOOP=(7,0){SDA=ADR[I],SCL=P}     ; send the cell address
SDA=0,SDA=1,SCL=1,SDA?0          ; ACK
SCL=0,SDA=0
_START                           ; repeated Start
LOOP=(7,0){SDA=R11[I],SCL=P}     ; control byte 0xA1 (read)
SDA=0,SDA=1,SCL=1,SDA?0          ; ACK
SCL=0
SDA=1                            ; release SDA (Hi-Z), chip drives now
LOOP=(7,0){SCL=1,DATA[I]=SDA,SCL=0}   ; read 8 bits into DATA
SDA=1,SCL=1,SCL=0,SDA=0          ; NACK (master wants no more)
SCL=0,SCL=1,SDA=1                ; Stop
```

Note the **"ACK check"** pattern: after each byte sent, the master releases SDA and checks the chip pulled the line to 0 (`SDA?0`).

**Write (`[WRITE]`)** ends with a wait for the internal write cycle:

```hpl
[WRITE]
_START
LOOP=(7,0){SDA=R10[I],SCL=P}     ; 0xA0
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
LOOP=(7,0){SDA=ADR[I],SCL=P}     ; address
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
LOOP=(7,0){SDA=DATA[I],SCL=P}    ; data
SDA=0,SDA=1,SCL=1,SDA?0
SCL=0,SDA=0
SCL=0,SCL=1,SDA=1                ; Stop
P=$WDELAY                        ; wait for write (25 ms)
```

---

## 12. Case study 2 — Microwire 93Cxx

**`9306.hpl`** (NMC9306, 16×16, Microwire) shows a different protocol: 3 lines + instructions.

**Pins and mode:**

```hpl
SOCKET=2 ;"MW"
PINO=CLK,0
PINO=DI,1        ; chip's data-in (we drive)
PINO=CS,2        ; chip select
PINI=DO,1        ; chip's data-out (we read)
CDELAY=8
```

**Read** — the READ instruction = start bit `1` + opcode `10` + address:

```hpl
[READ]
CS=0
CLK=0
DI=0
CS=1
R0=011000B                        ; startbit+opcode(10)+... (see datasheet)
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}  ; send the instruction (6 bits)
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0} ; send the address (4 bits)
DI=1
LOOP=(15,0){CLK=1,CLK=0,DATA[I]=DO}  ; read a 16-bit word from DO
CS=0
```

**Write** needs enable (`EWEN`), then `ERASE`, `WRITE`, then disable (`EWDS`):

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
R0=011100B                        ; ERASE + address
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0}
CS=0
P=20000                           ; wait for erase

DI=0
CLK=0
CS=1
R0=010100B                        ; WRITE + address + data
LOOP=(5,0){DI=R0[I],CLK=1,CLK=0}
LOOP=(3,0){DI=ADR[I],CLK=1,CLK=0}
LOOP=(15,0){CLK=0,DI=DATA[I],CLK=1}  ; 16 data bits
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

In Microwire, "instructions" are just bit strings you clock out on `DI`.

---

## 13. Case study 3 — SPI 25Cxx

**`25c256.hpl`** (SPI EEPROM, 16-bit address). Four-line pattern: SCK, SI (MOSI), SO (MISO), CS.

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

[_SEND]                            ; send byte from R0 (MSB first)
LOOP=(7,0){SI=R0[I],SCK=P}

[READ]
CS=1
SCK=0
CS=0
_SEND(00000011b)                   ; READ command (0x03)
LOOP=(15,0){SI=ADR[I],SCK=P}       ; 16-bit address
SI=1
R0=0
LOOP=(7,0){SCK=1,R0[I]=SO,SCK=0}   ; read a byte from SO
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
LOOP=(15,0){SI=ADR[I],SCK=P}       ; address
LOOP=(7,0){SI=DATA[I],SCK=P}       ; data
SI=1,CS=1
P=$WDELAY                          ; wait for write
```

Comparing the three protocols:

| | Lines | Order |
|--|-------|-------|
| **I2C** | SDA, SCL (2, open-drain) | Start → addr+RW → ACK → data → ACK → Stop |
| **Microwire** | CLK, DI, DO, CS (4) | CS↑ → startbit+opcode+addr → data |
| **SPI** | SCK, SI, SO, CS (4) | CS↓ → command → addr → data → CS↑ |

In all three, the core is the same: **a `LOOP` sending/receiving bits with clock toggling**.

---

## 14. Registering and distributing scripts

To make your script appear in the Orange5 menu, add an entry to any `*.cfg` file (Orange5 scans them all):

```
GROUP=MyScripts
CHIP=My Chip,256,my_script.hpl
```

- `GROUP=` — menu group name.
- `CHIP=<name>,<size>,<file>` — menu item. Size e.g. `256`, `1K`, `64x16`, `32K`.
- Put the `.hpl` file in the `HPL\` folder.

**Sharing with others:** just give them the `.hpl` file + the `CHIP=` line. Encryption (`.hpx`) is **not required** — plain `.hpl` works the same and mixes with `.hpx` in configs. Safe pattern: your own `Z-MyScripts.cfg` (loads last, leaves vendor files untouched).

---

## 15. Best practices and debugging

- **Start with `[INIT]`** that sets all pins to a safe state.
- **Check ACK/response** (`SDA?0`, poll a ready bit) and report errors via `PRINT=E(...)`.
- **`CDELAY`** tunes speed — if the chip won't respond, increase it.
- **`P=$WDELAY`** after a write — memories need time for the internal cycle.
- **Comment** each block (Start, ACK, address, data) — like the original scripts.
- **Test the logic** in the **HPL Studio** simulator (step execution, register and pin watch) before touching hardware.
- **`hpltest.hpl`** in the HPL folder is living documentation — it exercises every keyword.

---

## 16. Cheat sheet (appendix)

```
; --- header ---
INFO="..."   SOCKET=n   CDELAY=n   VCC=mV   BAUD=n

; --- pins ---
PINO=NAME,n   PINI=NAME,n   PING=NAME,n   PINO=#NAME,n
NAME=1 / =0 / =P    NAME=R0[i]

; --- registers ---
R0..RF  DATA  ADR  RA  I    R0[i]  DATA[i]  ADR[i]
R9=NAME,format,bits...       $MACRO

; --- numbers ---
0x1F   1FH   1010B   31

; --- math (in place on left register) ---
=+  =-  =&  =|  =^  =<<  =>>  =*  =/  =%     (constant or Rx)

; --- control ---
LOOP=(a,b){ }   LOOP=(N){ }   LOOP(Rx){ }
X?v{ }  X?!v{ }  X?<v{ }  X?>v{ }
BREAK   EXIT   P=n

; --- subroutines ---
[_NAME]  ...          _NAME        _NAME(arg->R0)

; --- dialogs ---
PRINT=("..")  I(..) E(..) A(..) S(..) L(..)
GET=("..",Rx)         %lu %08lX \n        RA (1/0)

; --- operation sections ---
[!SETUP] [INIT] [READ] [READBLOCK] [WRITE] [WRITEBLOCK] [ERASE] [END]

; --- registration (.cfg) ---
GROUP=Name
CHIP=Name,Size,file.hpl
```

---

*Built from authentic scripts `24c02.hpl`, `9306.hpl`, `25c256.hpl`, `hpltest.hpl` and others from the Orange5 distribution.*
