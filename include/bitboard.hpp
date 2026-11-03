#pragma once

#include <cstdint>
#include <array>
#include <bit>
#include <string>

namespace chess {

// ---------- Constants ----------
constexpr int SQUARES = 64;
constexpr int COLORS = 2;
constexpr int PIECE_TYPES = 6;
constexpr int MAX_MOVES = 256;

// ---------- Enums ----------
enum class Color : uint8_t { White = 0, Black = 1 };
enum class PieceType : uint8_t { Pawn = 0, Knight = 1, Bishop = 2, Rook = 3, Queen = 4, King = 5 };
enum class Square : uint8_t {
  A1 = 0, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  None = 255
};
enum class File : uint8_t { FileA = 0, FileB, FileC, FileD, FileE, FileF, FileG, FileH };
enum class Rank : uint8_t { Rank1 = 0, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8 };

constexpr Color operator~(Color c) { return Color(static_cast<uint8_t>(c) ^ 1); }

// ---------- Bitboard ----------
using Bitboard = uint64_t;

constexpr Bitboard BB_SQUARES[SQUARES] = {
  0x1ULL, 0x2ULL, 0x4ULL, 0x8ULL, 0x10ULL, 0x20ULL, 0x40ULL, 0x80ULL,
  0x100ULL, 0x200ULL, 0x400ULL, 0x800ULL, 0x1000ULL, 0x2000ULL, 0x4000ULL, 0x8000ULL,
  0x10000ULL, 0x20000ULL, 0x40000ULL, 0x80000ULL, 0x100000ULL, 0x200000ULL, 0x400000ULL, 0x800000ULL,
  0x1000000ULL, 0x2000000ULL, 0x4000000ULL, 0x8000000ULL, 0x10000000ULL, 0x20000000ULL, 0x40000000ULL, 0x80000000ULL,
  0x100000000ULL, 0x200000000ULL, 0x400000000ULL, 0x800000000ULL, 0x1000000000ULL, 0x2000000000ULL, 0x4000000000ULL, 0x8000000000ULL,
  0x10000000000ULL, 0x20000000000ULL, 0x40000000000ULL, 0x80000000000ULL, 0x100000000000ULL, 0x200000000000ULL, 0x400000000000ULL, 0x800000000000ULL,
  0x1000000000000ULL, 0x2000000000000ULL, 0x4000000000000ULL, 0x8000000000000ULL, 0x10000000000000ULL, 0x20000000000000ULL, 0x40000000000000ULL, 0x80000000000000ULL,
  0x100000000000000ULL, 0x200000000000000ULL, 0x400000000000000ULL, 0x800000000000000ULL, 0x1000000000000000ULL, 0x2000000000000000ULL, 0x4000000000000000ULL, 0x8000000000000000ULL
};

constexpr Bitboard FILES[8] = {
  0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
  0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
};

constexpr Bitboard RANKS[8] = {
  0x00000000000000FFULL, 0x000000000000FF00ULL, 0x0000000000FF0000ULL, 0x00000000FF000000ULL,
  0x000000FF00000000ULL, 0x0000FF0000000000ULL, 0x00FF000000000000ULL, 0xFF00000000000000ULL
};

// Bitboard operations
inline constexpr Bitboard bb_square(Square s) { return BB_SQUARES[static_cast<int>(s)]; }
inline constexpr int popcount(Bitboard b) { return std::popcount(b); }
inline constexpr int lsb_index(Bitboard b) { return std::countr_zero(b); }
inline constexpr Square pop_lsb(Bitboard& b) {
  int idx = lsb_index(b);
  b &= b - 1;
  return static_cast<Square>(idx);
}
inline constexpr Bitboard shift_n(Bitboard b) { return b << 8; }
inline constexpr Bitboard shift_s(Bitboard b) { return b >> 8; }
inline constexpr Bitboard shift_e(Bitboard b) { return (b & ~FILES[7]) << 1; }
inline constexpr Bitboard shift_w(Bitboard b) { return (b & ~FILES[0]) >> 1; }
inline constexpr Bitboard shift_ne(Bitboard b) { return shift_n(shift_e(b)); }
inline constexpr Bitboard shift_nw(Bitboard b) { return shift_n(shift_w(b)); }
inline constexpr Bitboard shift_se(Bitboard b) { return shift_s(shift_e(b)); }
inline constexpr Bitboard shift_sw(Bitboard b) { return shift_s(shift_w(b)); }

inline constexpr File file_of(Square s) { return File(static_cast<int>(s) & 7); }
inline constexpr Rank rank_of(Square s) { return Rank(static_cast<int>(s) >> 3); }
inline constexpr Square make_square(File f, Rank r) { return Square(static_cast<int>(r) * 8 + static_cast<int>(f)); }

// Print a bitboard as an 8x8 board (rank 8 to 1, file a to h)
// Useful for debugging: pass to std::cout << print_bb(bb)
std::string print_bb(Bitboard bb);

} // namespace chess