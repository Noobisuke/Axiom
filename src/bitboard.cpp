#include "bitboard.hpp"
#include <string>
#include <sstream>
#include <cstdint>
#include <array>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace chess {

// =====================================================================
// Debug printing
// =====================================================================
std::string print_bb(Bitboard bb) {
  std::ostringstream oss;
  oss << "\n";
  for (int r = 7; r >= 0; --r) {
    oss << (r + 1) << "  ";
    for (int f = 0; f < 8; ++f) {
      const Square sq = make_square(File(f), Rank(r));
      oss << ((bb & BB_SQUARES[static_cast<int>(sq)]) ? "1 " : ". ");
    }
    oss << "\n";
  }
  oss << "\n   a b c d e f g h\n";
  return oss.str();
}

// =====================================================================
// Leaper attack tables (knight, king, pawn)
// =====================================================================
constexpr int KNIGHT_OFFSETS[8][2] = {
  { 2,  1}, { 2, -1}, {-2,  1}, {-2, -1},
  { 1,  2}, { 1, -2}, {-1,  2}, {-1, -2}
};

constexpr int KING_OFFSETS[8][2] = {
  { 1,  0}, {-1,  0}, { 0,  1}, { 0, -1},
  { 1,  1}, { 1, -1}, {-1,  1}, {-1, -1}
};

Bitboard KNIGHT_ATTACKS[SQUARES];
Bitboard KING_ATTACKS[SQUARES];
Bitboard PAWN_ATTACKS[COLORS][SQUARES];

void init_attack_tables() {
  for (int sq = 0; sq < SQUARES; ++sq) {
    const int r = sq >> 3;
    const int f = sq & 7;

    Bitboard kn = 0;
    for (const auto& off : KNIGHT_OFFSETS) {
      const int nr = r + off[0];
      const int nf = f + off[1];
      if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) kn |= BB_SQUARES[nr * 8 + nf];
    }
    KNIGHT_ATTACKS[sq] = kn;

    Bitboard kg = 0;
    for (const auto& off : KING_OFFSETS) {
      const int nr = r + off[0];
      const int nf = f + off[1];
      if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) kg |= BB_SQUARES[nr * 8 + nf];
    }
    KING_ATTACKS[sq] = kg;

    Bitboard wp = 0, bp = 0;
    if (r < 7) {
      if (f > 0) wp |= BB_SQUARES[(r + 1) * 8 + (f - 1)];
      if (f < 7) wp |= BB_SQUARES[(r + 1) * 8 + (f + 1)];
    }
    if (r > 0) {
      if (f > 0) bp |= BB_SQUARES[(r - 1) * 8 + (f - 1)];
      if (f < 7) bp |= BB_SQUARES[(r - 1) * 8 + (f + 1)];
    }
    PAWN_ATTACKS[static_cast<int>(Color::White)][sq] = wp;
    PAWN_ATTACKS[static_cast<int>(Color::Black)][sq] = bp;
  }
}

// =====================================================================
// Sliding attacks: magic bitboards, magics generated and verified at startup
// =====================================================================

static Bitboard BISHOP_MASKS[SQUARES];
static Bitboard ROOK_MASKS[SQUARES];

Bitboard BISHOP_ATTACKS_TABLE[5248];
Bitboard ROOK_ATTACKS_TABLE[102400];

MagicEntry BISHOP_MAGICS[SQUARES];
MagicEntry ROOK_MAGICS[SQUARES];

static constexpr int BISHOP_TABLE_SIZE = 5248;
static constexpr int ROOK_TABLE_SIZE = 102400;

// Relevant-occupancy masks. Each ray stops ONE SQUARE BEFORE the board
// edge: occupancy on the edge square can never change the attack set, and
// including it would waste index bits.
//
// The tempting shortcut `rays & ~(FILES[0]|FILES[7]|RANKS[0]|RANKS[7])`
// is WRONG for a rook on an edge square -- e.g. on a1 the entire ray lies
// on the a-file / rank 1, so the mask collapses to 0. A zero mask means
// `shift == 64`, `>> 64` is undefined, and every lookup silently returns
// the empty-board attack set (rooks sliding through their own pawns).
static void init_masks() {
  for (int sq = 0; sq < SQUARES; ++sq) {
    const int r = sq >> 3;
    const int f = sq & 7;

    // Bishop: all four diagonals, stopping one square short of the edge.
    Bitboard bm = 0;
    for (int i = 1; r + i <= 6 && f + i <= 6; ++i) bm |= BB_SQUARES[(r + i) * 8 + (f + i)];
    for (int i = 1; r + i <= 6 && f - i >= 1; ++i) bm |= BB_SQUARES[(r + i) * 8 + (f - i)];
    for (int i = 1; r - i >= 1 && f + i <= 6; ++i) bm |= BB_SQUARES[(r - i) * 8 + (f + i)];
    for (int i = 1; r - i >= 1 && f - i >= 1; ++i) bm |= BB_SQUARES[(r - i) * 8 + (f - i)];
    BISHOP_MASKS[sq] = bm;

    // Rook: all four orthogonals, stopping one square short of the edge.
    Bitboard rm = 0;
    for (int x = r + 1; x <= 6; ++x) rm |= BB_SQUARES[x * 8 + f];
    for (int x = r - 1; x >= 1; --x) rm |= BB_SQUARES[x * 8 + f];
    for (int x = f + 1; x <= 6; ++x) rm |= BB_SQUARES[r * 8 + x];
    for (int x = f - 1; x >= 1; --x) rm |= BB_SQUARES[r * 8 + x];
    ROOK_MASKS[sq] = rm;
  }
}

// Ray-walking reference implementation. Magic tables are built from and
// verified against this, so it must stay obviously correct.
static Bitboard bishop_attacks_slow(Square sq, Bitboard occupied) {
  const int r = static_cast<int>(sq) >> 3;
  const int f = static_cast<int>(sq) & 7;
  Bitboard att = 0;

  for (int i = 1; r + i < 8 && f + i < 8; ++i) {
    const Bitboard t = BB_SQUARES[(r + i) * 8 + (f + i)];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; r + i < 8 && f - i >= 0; ++i) {
    const Bitboard t = BB_SQUARES[(r + i) * 8 + (f - i)];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; r - i >= 0 && f + i < 8; ++i) {
    const Bitboard t = BB_SQUARES[(r - i) * 8 + (f + i)];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; r - i >= 0 && f - i >= 0; ++i) {
    const Bitboard t = BB_SQUARES[(r - i) * 8 + (f - i)];
    att |= t;
    if (occupied & t) break;
  }
  return att;
}

static Bitboard rook_attacks_slow(Square sq, Bitboard occupied) {
  const int r = static_cast<int>(sq) >> 3;
  const int f = static_cast<int>(sq) & 7;
  Bitboard att = 0;

  for (int i = 1; r + i < 8; ++i) {
    const Bitboard t = BB_SQUARES[(r + i) * 8 + f];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; r - i >= 0; ++i) {
    const Bitboard t = BB_SQUARES[(r - i) * 8 + f];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; f + i < 8; ++i) {
    const Bitboard t = BB_SQUARES[r * 8 + (f + i)];
    att |= t;
    if (occupied & t) break;
  }
  for (int i = 1; f - i >= 0; ++i) {
    const Bitboard t = BB_SQUARES[r * 8 + (f - i)];
    att |= t;
    if (occupied & t) break;
  }
  return att;
}

// Enumerate every subset of `mask` (Carry-Rippler).
static void generate_occupancies(Bitboard mask, std::vector<Bitboard>& out) {
  out.clear();
  Bitboard subset = 0;
  do {
    out.push_back(subset);
    subset = (subset - mask) & mask;
  } while (subset);
}

// Deterministic splitmix64, so magic search is reproducible across runs.
static uint64_t g_magic_state = 0x9E3779B97F4A7C15ULL;

static uint64_t magic_rand() {
  g_magic_state += 0x9E3779B97F4A7C15ULL;
  uint64_t z = g_magic_state;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// Sparse random number (AND of three draws): a usable magic needs few bits.
static Bitboard sparse_rand() {
  return magic_rand() & magic_rand() & magic_rand();
}

// Search for a magic that maps every relevant occupancy to a collision-free
// index. Returns 0 only if the mask is empty (which must not happen).
static Bitboard find_magic(int sq, bool is_bishop) {
  const Bitboard mask = is_bishop ? BISHOP_MASKS[sq] : ROOK_MASKS[sq];
  const int bits = popcount(mask);
  if (bits == 0) return 0;
  const int size = 1 << bits;
  const int shift = 64 - bits;

  std::vector<Bitboard> occs;
  generate_occupancies(mask, occs);

  std::vector<Bitboard> ref(occs.size());
  for (size_t i = 0; i < occs.size(); ++i) {
    ref[i] = is_bishop ? bishop_attacks_slow(static_cast<Square>(sq), occs[i])
                       : rook_attacks_slow(static_cast<Square>(sq), occs[i]);
  }

  std::vector<Bitboard> used(static_cast<size_t>(size));
  std::vector<unsigned char> seen(static_cast<size_t>(size));

  for (;;) {
    const Bitboard magic = sparse_rand();
    if (popcount((mask * magic) >> 56) < 6) continue; // cheap reject

    std::fill(used.begin(), used.end(), 0);
    std::fill(seen.begin(), seen.end(), 0);

    bool ok = true;
    for (size_t i = 0; i < occs.size(); ++i) {
      const unsigned idx = static_cast<unsigned>((occs[i] * magic) >> shift);
      if (!seen[idx]) {
        seen[idx] = 1;
        used[idx] = ref[i];
      } else if (used[idx] != ref[i]) {
        ok = false;
        break;
      }
    }
    if (ok) return magic;
  }
}

// Build one side's tables. Returns false on any inconsistency so the caller
// can fail loudly instead of running with corrupt attack tables.
static bool build_magics(bool is_bishop) {
  MagicEntry* magics = is_bishop ? BISHOP_MAGICS : ROOK_MAGICS;
  Bitboard* table = is_bishop ? BISHOP_ATTACKS_TABLE : ROOK_ATTACKS_TABLE;
  const int capacity = is_bishop ? BISHOP_TABLE_SIZE : ROOK_TABLE_SIZE;

  int offset = 0;
  for (int sq = 0; sq < SQUARES; ++sq) {
    const Bitboard mask = is_bishop ? BISHOP_MASKS[sq] : ROOK_MASKS[sq];
    const int bits = popcount(mask);
    const int size = 1 << bits;

    if (bits == 0 || offset + size > capacity) return false;

    magics[sq].mask = mask;
    magics[sq].shift = 64 - bits;
    magics[sq].attacks = table + offset;
    magics[sq].reference = 0;

    const Bitboard magic = find_magic(sq, is_bishop);
    if (magic == 0) return false;
    magics[sq].magic = magic;

    std::vector<Bitboard> occs;
    generate_occupancies(mask, occs);

    for (const Bitboard occ : occs) {
      const Bitboard att = is_bishop ? bishop_attacks_slow(static_cast<Square>(sq), occ)
                                     : rook_attacks_slow(static_cast<Square>(sq), occ);
      const unsigned idx = static_cast<unsigned>((occ * magic) >> magics[sq].shift);
      magics[sq].attacks[idx] = att;
    }

    // Verify every slot round-trips, including interior-edge squares that
    // the naive mask formula used to break.
    for (const Bitboard occ : occs) {
      const Bitboard want = is_bishop ? bishop_attacks_slow(static_cast<Square>(sq), occ)
                                      : rook_attacks_slow(static_cast<Square>(sq), occ);
      const unsigned idx = static_cast<unsigned>((occ * magic) >> magics[sq].shift);
      if (magics[sq].attacks[idx] != want) return false;
    }

    offset += size;
  }
  return true;
}

void init_sliding_attacks() {
  init_masks();
  if (!build_magics(true) || !build_magics(false)) {
    std::fprintf(stderr, "FATAL: magic table construction failed\n");
    std::abort();
  }
}

} // namespace chess
