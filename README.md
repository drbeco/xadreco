# Xadreco

Chess engine by Dr. Beco, since 1988.

Copyright (C) 1988-2026 by Ruben Carlo Benante <rcb@beco.cc>

| | |
|---|---|
| **Version** | 9.0 |
| **Protocol** | UCI (Universal Chess Interface) |
| **Platforms** | Linux, Windows (cross-compiled) |
| **License** | GPL v2 |
| **Play online** | [lichess.org/@/Xadreco](https://lichess.org/@/Xadreco) |
| **Source** | [github.com/drbeco/xadreco](https://github.com/drbeco/xadreco) |

## Features

- UCI protocol (native, no wrapper)
- Opening book (livro.txt)
- Iterative deepening with alpha-beta pruning
- Fail-soft alpha-beta
- Non-blocking search with pthreads input
- Arena-based memory management
- Single-file C source (~3,800 lines)

## Build

Linux:

```bash
make xadreco
```

Windows (cross-compile from Linux):

```bash
make xadreco.exe
```

Requires: gcc (Linux), mingw-w64 (Windows cross-compile).

## Usage

Xadreco speaks UCI. Connect it to any UCI-compatible GUI:

- **Arena** (Windows): Engine > Install New Engine > select xadreco.exe
- **lichess-bot**: set `protocol: "uci"` and `executable: "xadreco -b livro.txt"` in config.yml
- **Command line**: `./xadreco -b livro.txt -v`

### Options

| Flag | Description |
|------|-------------|
| `-b <file>` | Opening book file |
| `-v` | Verbose (debug output to stderr) |
| `-vv` | Extra verbose (board display) |
| `-r <seed>` | Random mode |
| `-V` | Show version |
| `-VV` | Show version number only |
| `-h` | Help |

## History

Started as a hobby project in 1988. Version 0.1 dates from 1994 — a DOS
program with colored ASCII board and `conio.h`. First victory against its
creator in a memorable presentation to Computer Science classmates at UNESP.
Grew from 1,381 lines to a peak of 5,550 (v5.8, 2013), then refactored down
to ~3,800 lines (v9.0, 2026). Played on FICS (2013-2018), Lichess
(2018-present). UCI support added in v8.0 (2026), replacing the XBoard
protocol used since v2.0 (2004).

## License

GNU General Public License v2. See [COPYING](COPYING).


-------------------------

Have Fun!

Bom divertimento!

Beco.


