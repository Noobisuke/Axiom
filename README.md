# Axiom Chess Engine

[![CI](https://github.com/Noobisuke/Axiom/actions/workflows/ci.yml/badge.svg)](https://github.com/Noobisuke/Axiom/actions)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Axiom** is a UCI-compatible chess engine written in modern C++20, built from scratch with a focus on correctness, clarity, and performance.

## Current Status

🚧 **Early Development — Phase 3 complete (bitboards, legal move generation, perft verified)**

The engine generates fully legal moves and reproduces the standard perft counts
for all six reference positions, up to depth 6. It does **not play chess yet** —
there is no search beyond a placeholder and evaluation is material-only, so `go`
still returns `bestmove 0000`.

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
- ✅ Material-only evaluation (placeholder)
- ✅ Transposition table structure

### Perft verification

| Position | d1 | d2 | d3 | d4 | d5 | d6 |
|---|---|---|---|---|---|---|
| startpos | 20 | 400 | 8902 | 197281 | 4865609 | 119060324 |
| kiwipete | 48 | 2039 | 97862 | 4085603 | 193690690 | — |
| position 3 | 14 | 191 | 2812 | 43238 | 674624 | 11030083 |
| position 4 | 6 | 264 | 9467 | 422333 | 15833292 | — |
| position 5 | 44 | 1486 | 62379 | 2103487 | 89941194 | — |
| position 6 | 46 | 2079 | 89890 | 3894594 | 164075551 | — |

All 32 checks pass. (~25–34M nps, Release, Ryzen 5 5600H.)

## Not Yet Implemented

- ❌ Alpha-beta search, quiescence, iterative deepening
- ❌ Search enhancements (PVS, null move, LMR, futility, etc.)
- ❌ Draw detection (threefold repetition, 50-move rule, insufficient material)
- ❌ Handcrafted evaluation + Texel tuning
- ❌ NNUE evaluation
- ❌ Multithreading (Lazy SMP), time management, Syzygy TBs
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
├── CMakePresets.json       # Configure/build presets
├── configure.ps1           # PowerShell configure helper
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── STUDY_NOTES.md          # Development log
├── include/                # Public headers
│   ├── bitboard.hpp        # Bitboard types, constants, ops
│   ├── eval.hpp            # Evaluation interface
│   ├── move.hpp            # Move encoding (16-bit), MoveList
│   ├── movegen.hpp         # Move generation declarations
│   ├── position.hpp        # Position, FEN, validation
│   ├── search.hpp          # Search limits, info, entry point
│   ├── tt.hpp              # Transposition table
│   └── uci.hpp             # UCI protocol handler
├── src/                    # Implementation
├── tests/                  # Test executables
│   └── perft_test.cpp      # Perft verification (TODO)
├── bench/                  # Benchmark executable
│   └── bench.cpp           # Search bench (TODO)
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

### Debug Build (with sanitizers)

```powershell
cmake --preset debug
ninja -C build-debug
```

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

# Perft, depths 1-4 across all six reference positions
.\build\bin\perft_test.exe

# Go deeper (this is the real correctness gate; d6 takes ~4 s for startpos)
.\build\bin\perft_test.exe 6

# Per-move breakdown for one position, e.g. case 1 (kiwipete) at depth 3
.\build\bin\perft_test.exe 1 divide 3

# Bench (placeholder search until Phase 4)
.\build\bin\bench.exe

# UCI smoke test
printf 'uci\nisready\nposition startpos moves e2e4 e7e5 g1f3\ngo depth 3\nquit\n' | .\build\bin\axiom.exe
```

Or via CTest: `ctest --test-dir build --output-on-failure`

## Development Roadmap

| Phase | Focus | Target |
|-------|-------|--------|
| 3.1 | Attack tables | Knight, King, Pawn |
| 3.2 | Sliding attacks | PEXT + magic fallback |
| 3.3 | Move generation | Pseudo-legal + legal |
| 3.4 | Make/Unmake | Incremental Zobrist |
| 3.5 | Perft | Verify against known counts |
| 4 | Alpha-beta + qsearch | Basic playing strength |
| 5 | UCI + SPRT | fastchess integration |
| 6+ | PVS, null move, LMR, ... | Search enhancements |
| 7 | HCE + Texel tuning | Better evaluation |
| 8 | NNUE | Neural net evaluation |
| 9 | Lazy SMP, time mgmt | Multi-thread |
| 10-11 | Gauntlet vs Stockfish | Testing |

## License

MIT License — see [LICENSE](LICENSE) for details.

## Acknowledgements

- Inspired by open-source engines: **Stockfish**, **Ethereal**, **Weiss**, **Berserk**, **Vice/TSCP**
- Testing tools: **fastchess** (Disservin), **Stockfish** (Stockfish developers)
- Chess Programming Wiki for reference implementations