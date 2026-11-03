#pragma once

#include "bitboard.hpp"
#include <cstdint>

namespace chess {

// Move encoding: 16 bits
// bits 0-5:  from square (0-63)
// bits 6-11: to square (0-63)
// bits 12-14: promotion piece type (1=knight, 2=bishop, 3=rook, 4=queen), 0 = no promotion
// bit 15: special flag (en passant, castling, double pawn push)
struct Move {
  uint16_t data = 0;

  constexpr Move() = default;
  constexpr Move(uint16_t d) : data(d) {}

  static Move from_uci(const char* uci);

  constexpr Square from() const { return Square(data & 0x3F); }
  constexpr Square to() const { return Square((data >> 6) & 0x3F); }
  constexpr PieceType promotion() const { return PieceType((data >> 12) & 0x7); }
  constexpr bool is_promotion() const { return (data >> 12) != 0; }
  constexpr bool is_special() const { return data & 0x8000; }

  constexpr void set_from(Square s) { data = (data & ~0x3Fu) | static_cast<uint16_t>(static_cast<uint8_t>(s)); }
  constexpr void set_to(Square s) { data = (data & ~0xFC0u) | (static_cast<uint16_t>(static_cast<uint8_t>(s)) << 6); }
  constexpr void set_promotion(PieceType pt) { data = (data & ~0x7000u) | (static_cast<uint16_t>(static_cast<uint8_t>(pt)) << 12); }
  constexpr void set_special(bool v) { data = (data & ~0x8000u) | (static_cast<uint16_t>(v) << 15); }

  constexpr bool operator==(Move other) const { return data == other.data; }
  constexpr bool operator!=(Move other) const { return data != other.data; }
};

// Fixed-size move list (no heap allocation)
struct MoveList {
  std::array<Move, MAX_MOVES> moves{};
  size_t count = 0;

  constexpr void clear() { count = 0; }
  constexpr void add(Move m) { moves[count++] = m; }
  constexpr Move& operator[](size_t i) { return moves[i]; }
  constexpr const Move& operator[](size_t i) const { return moves[i]; }
  constexpr size_t size() const { return count; }
  constexpr bool empty() const { return count == 0; }
};

} // namespace chess