# Axiom Chess Engine

[![CI](https://github.com/Noobisuke/Axiom/actions/workflows/ci.yml/badge.svg)](https://github.com/Noobisuke/Axiom/actions)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Axiom** is a UCI-compatible chess engine written in modern C++20, built from scratch with a focus on correctness, clarity, and performance.

## Current Status

🚧 **Early Development — Phase 3, Step 2**  
Implementing: Position representation, robust FEN parsing/printing, position validation, Zobrist hashing.

## Goals

- Build a strong, readable chess engine from scratch
- Learn engine architecture through incremental development
- Ultimately test against Stockfish at various handicaps
- Clean, modern C++20 codebase (no legacy cruft)

## Implemented (Verified)

- ✅ Bitboard representation (uint64_t, popcount, LSB, shifts)
- ✅ UCI protocol (uci, isready, position, go, stop, quit)
- ✅ Basic engine loop and command parsing
- ✅ Material-only evaluation (placeholder)
- ✅ Transposition table structure
- 🚧 Position struct with mailbox, castling, EP, clocks, Zobrist, history
- 🚧 Robust FEN parser (4/6 fields, validation, error messages)
- 🚧 FEN printer (standard 6-field format)
- 🚧 Position validation (kings, pawns, piece counts, side-not-to-move check)

## Not Yet Implemented

- ❌ Attack tables (knight, king, pawn, sliding)
- ❌ Magic bitboard / PEXT sliding attacks
- ❌ Legal move generation
- ❌ Perft / perft-divide verification
- ❌ make_move / unmove_move with incremental updates
- ❌ Alpha-beta search, quiescence, PVS
- ❌ Search enhancements (null move, LMR, futility, etc.)
- ❌ Handcrafted evaluation + Texel tuning
- ❌ NNUE evaluation
- ❌ Multithreading (Lazy SMP), time management, Syzygy TBs

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
# Perft tests (once implemented)
.\build\bin\perft_test.exe

# Bench (once search exists)
.\build\bin\bench.exe

# UCI smoke test
echo "uci`nisready`nposition startpos`ngo depth 10`nquit" | .\build\bin\axiom.exe
```

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