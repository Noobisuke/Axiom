# Study Notes - Chess Engine Development

## Phase 3, Step 1: Bitboard Types and Operations

### Key Ideas
- **Bitboard**: 64-bit integer where each bit represents a square (0 = empty, 1 = occupied)
- **Square mapping**: A1=0, B1=1, ..., H1=7, A2=8, ..., H8=63 (little-endian rank-file)
- **LSB = index 0 = A1**: `std::countr_zero` finds least significant set bit efficiently
- **pop_lsb pattern**: `idx = countr_zero(bb); bb &= bb - 1;` removes LSB and returns its index
- **Shift operations**: Pre-computed with file masking (e.g., `shift_e` masks off H-file before `<< 1`)

### Common Mistakes
1. **Shift wrapping**: Forgetting to mask files before east/west shifts causes H-file pieces to wrap to A-file
2. **Signed vs unsigned**: Use `uint64_t` everywhere; shifting signed integers is UB
3. **Shift by 64**: `bb << 64` or `bb >> 64` is UB in C++; guard against it
4. **std::popcount/countr_zero**: Require C++20 `<bit>` header; not available in older standards

### Testing Notes
- `print_bb()` orientation: Rank 8 at top, Rank 1 at bottom, File a left, File h right
- All 8 directional shifts verified from center (E4) and edge (H1)
- `popcount`, `lsb_index`, `pop_lsb` round-trip correctly
- `file_of`, `rank_of`, `make_square` inverses verified

### Files Modified
- `include/bitboard.hpp`: Added `print_bb()` declaration, restored `file_of`/`rank_of`
- `src/bitboard.cpp`: Implemented `print_bb()`
- Test: `test_step1.cpp` (ad-hoc verification)

---

## Phase 3, Step 2: Position Struct, FEN Parsing/Printing, Validation

### Key Ideas
- **Position struct** holds all game state: piece bitboards per color/type, mailbox array for O(1) square lookup, side to move, castling rights (4 bits), en passant square, halfmove/fullmove clocks, Zobrist hash, history stack for repetition detection
- **FEN parsing**: Robust tokenization on whitespace, accepts 4 or 6 fields, validates each field strictly, returns error messages
- **FEN printing**: Standard 6-field format, castling in KQkq order, `-` for empty fields
- **Validation**: Exactly 1 king/side, no adjacent kings, pawn counts ≤8, piece counts ≤16, no pawns on back ranks, side-not-to-move not in check, castling rights match board (drop invalid rights), mailbox≡bitboards consistency

### Common Mistakes
1. **FEN parsing fragility**: Assuming single spaces; must handle multiple spaces/tabs
2. **Invalid castling rights**: FEN may claim KQkq but king/rooks moved; drop the right silently (standard behavior)
3. **Stale en passant**: FEN may have EP square when no capture possible; set to None, don't reject
4. **Halfmove clock overflow**: Guard against large values; don't reject >100, just warn
5. **Zobrist initialization**: Use fixed random keys for deterministic testing

### Testing Notes
- Round-trip: parse → print equals input for all 6 standard test FENs
- 4-field FEN parses with default clocks
- Invalid FENs rejected with clear errors (7 ranks, 9 squares/rank, unknown pieces, 2 white kings, pawn on rank 1, side "x", castling "Z", EP "e9", side-not-to-move in check)
- Mailbox/bitboard consistency verified
- Zobrist key changes with position changes

### Files Modified
- `include/types.hpp`: New file with Piece encoding and helpers
- `include/position.hpp`: Added mailbox, zobrist_key, history arrays, new methods
- `src/position.cpp`: Complete rewrite with robust parser, validator, printer
- `tests/fen_test.cpp`: New test executable

---

## Phase 3, Step 3: Attack Tables (Knights, Kings, Pawns)

### Key Ideas
- **Precomputed attack tables**: 64-entry arrays for knights, kings; 2×64 for pawns (color-specific)
- **Knight/King**: Generated from fixed (dr,df) offsets with bounds checking
- **Pawns**: Color-dependent; White attacks NW/NE (rank+1), Black attacks SW/SE (rank-1)
- **Initialization**: Single `init_attack_tables()` call at program startup
- **Total moves verified**: Knights=336, Kings=420, Pawns=98 each (correct accounting for board edges)

### Common Mistakes
1. **Pawn attack direction**: White attacks "up" (rank+1), Black attacks "down" (rank-1) — easy to flip
2. **Edge file handling**: Pawns on A-file only attack right, H-file only attack left
3. **Rank bounds**: No attacks from 1st/8th rank for respective colors
4. **Const correctness**: Header declares `extern const` but cpp defines non-const (initialized at runtime) → remove `const` from extern declaration

### Testing Notes
- Visual verification with `print_bb()` for center (E4) and corner (A1) squares
- Total move counts match theoretical values
- All 64 squares tested programmatically

### Files Modified
- `include/bitboard.hpp`: Added attack table declarations and `init_attack_tables()`
- `src/bitboard.cpp`: Implemented attack table generation
- `src/main.cpp`: Call `init_attack_tables()` at startup
- Test: `test_attack_tables.cpp` (ad-hoc verification)

---

## Phase 3, Step 4: Sliding Attacks (Magic Bitboards & PEXT)

### Key Ideas
- **Magic bitboards**: Precompute attacks for all possible blocker configurations per square using minimal perfect hashing
- **Bishop mask**: Diagonal rays excluding edges (6 bits max → 64 entries per square)
- **Rook mask**: Orthogonal rays excluding edges (12 bits max → 4096 entries per square)
- **Magic numbers**: Fixed constants (from Stockfish/wiki) mapping occupancy to attack index
- **PEXT alternative**: BMI2 `pext` instruction directly extracts relevant bits; faster on Zen 3, fallback to magic bitboards
- **Total table sizes**: Bishop 5,248 entries, Rook 102,400 entries

### Common Mistakes
1. **Edge squares in mask**: Occupancy on edges never blocks sliding pieces; must clear FILES[0], FILES[7], RANKS[0], RANKS[7] from mask
2. **Shift calculation**: `shift = 64 - index_bits`; wrong shift breaks indexing
3. **Table population**: Must iterate ALL subsets of mask (not just random ones) using `(subset - mask) & mask` pattern
4. **Slow verification**: Ray-tracing reference (`bishop_attacks_slow`, `rook_attacks_slow`) essential for validating magic tables
5. **UB in shifts**: `magic * occupancy` can overflow 64-bit; cast to unsigned `__int128` or rely on modulo 2^64 behavior

### Testing Notes
- Empty board: Bishop E4=13, Rook E4=14, Queen E4=27 moves
- Blockers correctly stop rays (E5 blocks rook north, F5 blocks bishop NE)
- Corner squares (A1, H8) verified
- All 64 squares tested programmatically

### Files Modified
- `include/bitboard.hpp`: Added sliding attack declarations, magic structs, PEXT config
- `src/bitboard.cpp`: Implemented `init_sliding_attacks()`, ray masks, magic table population, PEXT functions
- `src/main.cpp`: Call `init_sliding_attacks()` at startup
- Test: `test_attack_tables.cpp` (ad-hoc verification)

---

## Phase 3, Step 2 (completed late): Position, robust FEN, validation

### Key Ideas
- `Position` now carries mailbox + bitboards kept in sync via `put_piece`/`remove_piece`/`move_piece`; `recompute_from_mailbox()` proves the two agree
- `set_fen` returns `std::string` ("" == success), tokenises on whitespace, accepts 4/6 fields, and validates before returning
- En passant: target must be on the correct rank (6 for White to move, 3 for Black), empty, and have the double-pushed enemy pawn directly behind it; a *valid* target is preserved even when no capture is possible
- Castling: a right not backed by king+rook on home squares is **rejected**, not silently dropped (per the Step 2 test list)
- Zobrist: splitmix64-derived fixed keys; `compute_key()` from scratch, incremental updates in make/unmake
- History: `history_keys`/`history_halfmoves` (1024) filled by make_move; repetition *logic* still pending

### Common Mistakes
1. `p += 2` style FEN parsing breaks on double spaces/tabs — tokenise instead
2. Forgetting that 4-field FENs are legal (clocks default to 0 and 1)
3. Silently "repairing" a FEN hides input bugs; decide drop-vs-reject and be consistent

---

## Phase 3, Step 5/6/7: Move generation, make/unmake, perft

### Key Ideas
- Pseudo-legal generation from attack tables, then a **make → test king safety → unmake** filter. This handles pins, check evasions and the en-passant discovered check on a rank with no special cases
- Castling is generated pre-validated: rights present, path empty, king not in check, and not passing through/onto an attacked square (b1/b8 may be attacked on queenside)
- Pawn attacks reuse `PAWN_ATTACKS`: our pawns attacking `sq` == `PAWN_ATTACKS[them][sq] & our_pawns`
- `attackers_to()` aggregates leaper tables + `bishop_attacks`/`rook_attacks` against queens as well as bishops/rooks
- make/unmake saves an `Undo` (captured piece + its square, castling, ep, halfmove clock, history count, zobrist key); ep capture square differs from `to()`
- Perft(1) returns the legal move count directly, avoiding a full extra ply of make/unmake

### Common Mistakes
1. **Relevant-occupancy masks for edge squares.** `rays & ~(FILES[0]|FILES[7]|RANKS[0]|RANKS[7])` is fine for bishops but WRONG for a rook on an edge square: its whole ray lies on the edge, the mask becomes 0, `shift` becomes 64, `>> 64` is UB and the lookup returns the empty-board attack set. Correct mask stops one square *before* the edge. Symptom: rooks sliding through their own pawns (`a1a3..a1a8`).
2. Trusting a copied magic-number table. A bad constant collides silently. Generate magics at startup and verify every occupancy round-trips.
3. Testing attacks only from a central square (E4) hides edge-square mask bugs.
4. Using bit 15 of the move word as a flag breaks `is_promotion()` (`data >> 12` picks up that bit).

### Verification (all PASS)
| Position | d1 | d2 | d3 | d4 | d5 | d6 |
|---|---|---|---|---|---|---|
| startpos | 20 | 400 | 8902 | 197281 | 4865609 | 119060324 |
| kiwipete | 48 | 2039 | 97862 | 4085603 | 193690690 | — |
| position3 | 14 | 191 | 2812 | 43238 | 674624 | 11030083 |
| position4 | 6 | 264 | 9467 | 422333 | 15833292 | — |
| position5 | 44 | 1486 | 62379 | 2103487 | 89941194 | — |
| position6 | 46 | 2079 | 89890 | 3894594 | 164075551 | — |

32/32 checks pass. Throughput ~25–34M nps in Release on the Ryzen 5 5600H.

### Files Modified
- `include/types.hpp`: Piece encoding, castling bits, Zobrist declarations
- `include/position.hpp` / `src/position.cpp`: mailbox, hash, history, robust FEN, `to_fen`, `validate`, `attackers_to`, `in_check`
- `include/movegen.hpp` / `src/movegen.cpp`: `Undo`, legal generation, make/unmake, `perft`, `perft_divide`, `move_to_uci`
- `src/bitboard.cpp`: runtime-generated + verified magics, corrected edge-square masks
- `tests/fen_test.cpp` (new), `tests/perft_test.cpp` (rewritten)
- `src/main.cpp`: also initialises sliding attacks and Zobrist

---