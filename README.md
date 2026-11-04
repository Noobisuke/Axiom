# Axiom Chess Engine

[![CI](https://github.com/Noobisuke/Axiom/actions/workflows/ci.yml/badge.svg)](https://github.com/Noobisuke/Axiom/actions)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Axiom** is a UCI-compatible chess engine written in modern C++20, built from scratch with a focus on correctness, clarity, and performance.

## Current Status

🚧 **Early Development — Phase 3 complete, Phase 4 in progress**

Phase 3 (bitboards, legal move generation) is done and perft-verified: all six
reference positions match the published counts at every listed depth (32/32
checks, ~36M nps single-core). Phase 4 is underway: static evaluation
(material + piece-square tables, side-to-move perspective) and a negamax
alpha-beta search with iterative deepening and mate-distance scores are
implemented and tested. The engine now plays legal moves over UCI — quiescence
search, move ordering, a transposition table, and draw detection are the
remaining Phase 4 steps.

## Goals

- Build a strong, readable chess engine from scratch
- Learn engine architecture through incremental development
- Ultimately test against Stockfish at various handicaps
- Clean, modern C++20 codebase (no legacy cruft)

## Implemented (Verified)

- ✅ Bitboard representation (uint64_t, popcount, LSB, shifts, `print_bb`)
- ✅ Precomputed attack tables: knight, king, pawn
- ✅ Sliding attacks via magic bitboards, magics generated **and verified** at startup
- ✅ Position with piece/colour bitboards, mailbox, castling rights, en passant, clocks
- ✅ Robust FEN parser (4- or 6-field, whitespace-tolerant, error messages) and 6-field printer
- ✅ Position validation (one king per side, no adjacent kings, pawn/​material limits, side-not-to-move in check, mailbox≡bitboards)
- ✅ Zobrist hashing (fixed keys, computed from scratch and updated incrementally)
- ✅ Fully legal move generation (pins, checks, en passant, castling, promotions)
- ✅ make / unmake with an `Undo` record and incremental hash
- ✅ perft and perft-divide
- ✅ UCI protocol (uci, isready, position, ucinewgame, go, stop, quit)
- ✅ Static evaluation: material + piece-square tables, side-to-move perspective (`eval_test`: 8 checks)
- ✅ Negamax alpha-beta search with iterative deepening, mate-distance scores, node/time limits, PV output (`search_test`: 7 checks, mate-in-1/2/3 verified against Stockfish)
- ✅ Transposition table structure (not yet probed by search)

### Perft verification

| Position | d1 | d2 | d3 | d4 | d5 | d6 |
|---|---|---|---|---|---|---|
| startpos | 20 | 400 | 8902 | 197281 | 4865609 | 119060324 |
| kiwipete | 48 | 2039 | 97862 | 4085603 | 193690690 | — |
| position 3 | 14 | 191 | 2812 | 43238 | 674624 | 11030083 |
| position 4 | 6 | 264 | 9467 | 422333 | 15833292 | — |
| position 5 | 44 | 1486 | 62379 | 2103487 | 89941194 | — |
| position 6 | 46 | 2079 | 89890 | 3894594 | 164075551 | — |

All 32 checks pass. (~36M nps, Release, Ryzen 5 5600H.)

## Not Yet Implemented

- ❌ Quiescence search (Phase 4c)
- ❌ Move ordering: MVV-LVA captures first (Phase 4d)
- ❌ Transposition table probing in search (Phase 4e)
- ❌ Draw detection: threefold repetition, 50-move rule, insufficient material (Phase 4f)
- ❌ Search enhancements (PVS, null move, LMR, futility, etc.)
- ❌ Handcrafted evaluation + Texel tuning
- ❌ NNUE evaluation
- ❌ Multithreading (Lazy SMP), real time management, Syzygy TBs
- ❌ PEXT-based sliding attacks (magic bitboards are the current, portable path)

## Technology Stack

- **Language:** C++20
- **Build:** CMake 3.25+ with Ninja
- **Compiler:** GCC 13+ (MinGW-w64) or Clang 16+ on Windows
- **Target:** AVX2 / BMI2 (znver3 for Ryzen 5000)
- **Testing:** fastchess + Stockfish for SPRT

## Project Directory Structure

```
Axiom/
├── CMakeLists.txt          # Build configuration
├── CMakePresets.json       # Configure/build presets (release, debug, relwithdebinfo)
├── configure.ps1           # PowerShell configure helper
├── README.md
├── ROADMAP.md              # Full 12-phase development roadmap
├── LICENSE
├── CONTRIBUTING.md
├── STUDY_NOTES.md          # Development log
├── include/                # Public headers
│   ├── bitboard.hpp        # Bitboard types, constants, ops, attack tables, magics
│   ├── types.hpp           # Piece encoding, castling bits, Zobrist keys
│   ├── move.hpp            # Move encoding (16-bit), MoveList
│   ├── position.hpp        # Position, FEN, validation, attack queries
│   ├── movegen.hpp         # Legal movegen, make/unmake, perft
│   ├── eval.hpp            # Evaluation interface (side-to-move perspective)
│   ├── search.hpp          # Search limits, info, negamax, mate score constants
│   ├── tt.hpp              # Transposition table
│   └── uci.hpp             # UCI protocol handler
├── src/                    # Implementation (.cpp per header)
├── tests/                  # Test executables (built by CMake, run via CTest)
│   ├── fen_test.cpp        # FEN parsing, validation, Zobrist (48 checks)
│   ├── perft_test.cpp      # Perft vs published counts (32 checks at full depth)
│   ├── eval_test.cpp       # Eval: symmetry, stm perspective, PST mirroring
│   ├── search_test.cpp     # Search: mate in 1/2/3, stalemate, limits
│   └── scratch/            # (gitignored) ad-hoc dev scratch files
├── bench/                  # Benchmark executable
│   └── bench.cpp
├── .github/                # GitHub workflows & templates
└── tools/                  # (gitignored) fastchess, Stockfish, ninja
```

## Building

### Prerequisites

- Windows 10/11
- Git
- MinGW-w64 GCC 13+ (UCRT64) or LLVM Clang 16+
- CMake 3.25+, Ninja
- Python 3 (for some test scripts)

### Quick Start (PowerShell)

```powershell
# Clone
git clone https://github.com/Noobisuke/Axiom.git
cd Axiom

# Configure (Release)
.\configure.ps1

# Build
ninja -C build

# Run engine
.\build\bin\axiom.exe
```

### Debug Build

```powershell
cmake --preset debug
ninja -C build-debug
```

Note: the preset enables ASan/UBSan, which requires the sanitizer runtimes
(`libasan`/`libubsan`). The current MinGW-w64 GCC 16.1 install does not ship
them, so linking the debug preset fails here. As a local substitute, perft was
also verified under `-D_GLIBCXX_ASSERTIONS -D_GLIBCXX_DEBUG -O1` (30/30 checks,
no assertion failures).

### Presets (CMakePresets.json)

| Preset | Description |
|--------|-------------|
| `release` | -O3, LTO, znver3, no asserts |
| `debug` | -O0, ASan/UBSan, full debug info |
| `relwithdebinfo` | -O2, debug symbols, no sanitizers |

## Running Tests

```powershell
# FEN parsing, validation, mailbox/bitboard agreement, Zobrist  (48 checks)
.\build\bin\fen_test.exe

# Eval: symmetry, side-to-move perspective, PST mirroring  (8 checks)
.\build\bin\eval_test.exe

# Search: mate in 1/2/3, stalemate, hanging queen, limits  (7 checks)
.\build\bin\search_test.exe

# Perft, depths 1-4 across all six reference positions
.\build\bin\perft_test.exe

# Go deeper (this is the real correctness gate; d6 takes ~4 s for startpos)
.\build\bin\perft_test.exe 6

# Per-move breakdown for one position, e.g. case 1 (kiwipete) at depth 3
.\build\bin\perft_test.exe 1 divide 3

# Bench
.\build\bin\bench.exe

# UCI smoke test
printf 'uci\nisready\nposition startpos moves e2e4 e7e5 g1f3\ngo depth 6\nquit\n' | .\build\bin\axiom.exe
```

Or via CTest: `ctest --test-dir build --output-on-failure`

### Match testing (fastchess vs Stockfish)

`tools/` (gitignored) contains fastchess and Stockfish. A quick smoke match:

```powershell
.\tools\fastchess-windows-x86-64\fastchess.exe `
  -engine name=Axiom cmd=.\build\bin\axiom.exe `
  -engine name=Stockfish cmd=.\tools\stockfish\stockfish-windows-x86-64-avx2.exe `
  -each nodes=20000 -games 16 -concurrency 4
```

For real measurement use a time control and paired openings (Phase 5):
`-each tc=10+0.1 -games 200 -openings file=book.epd -openings order=random`.
Note: with very short `st=` movetimes Axiom may lose on time — proper time
management is a Phase 9 topic.

## Development Roadmap

| Phase | Focus | Status |
|-------|-------|--------|
| 3 | Bitboards, legal movegen, perft | ✅ done (32/32 perft checks) |
| 4a | Static eval (material + PST, stm perspective) | ✅ done (8 eval checks) |
| 4b | Negamax alpha-beta + iterative deepening, mate scores | ✅ done (7 search checks) |
| 4c-f | Quiescence, move ordering, TT, draw detection | 🚧 in progress |
| 5 | UCI + bench + SPRT harness | ⬜ |
| 6+ | PVS, null move, LMR, ... | ⬜ |
| 7 | HCE + Texel tuning | ⬜ |
| 8 | NNUE | ⬜ |
| 9 | Lazy SMP, time mgmt | ⬜ |
| 10-11 | Gauntlet vs Stockfish | ⬜ |

## License

MIT License — see [LICENSE](LICENSE) for details.

## Acknowledgements

- Inspired by open-source engines: **Stockfish**, **Ethereal**, **Weiss**, **Berserk**, **Vice/TSCP**
- Testing tools: **fastchess** (Disservin), **Stockfish** (Stockfish developers)
- Chess Programming Wiki for reference implementations