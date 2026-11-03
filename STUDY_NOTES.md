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