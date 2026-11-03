#pragma once

#include "bitboard.hpp"
#include <array>

namespace chess {

struct Position {
  // Piece bitboards: [color][piece_type]
  std::array<std::array<Bitboard, PIECE_TYPES>, COLORS> pieces{};
  // Occupancy by color
  std::array<Bitboard, COLORS> by_color{};
  // All pieces
  Bitboard occupied = 0;
  // Side to move
  Color side_to_move = Color::White;
  // Castling rights (bit 0: WK, 1: WQ, 2: BK, 3: BQ)
  uint8_t castling = 0;
  // En passant square
  Square ep_square = Square::None;
  // Halfmove clock (for 50-move rule)
  int halfmove_clock = 0;
  // Fullmove number
  int fullmove_number = 1;

  // Reset to start position
  void set_startpos();

  // FEN parsing
  bool set_fen(const char* fen);

  // Occupancy helpers
  constexpr Bitboard pieces_of(Color c, PieceType pt) const { return pieces[static_cast<int>(c)][static_cast<int>(pt)]; }
  constexpr Bitboard all_pieces(Color c) const { return by_color[static_cast<int>(c)]; }
};

} // namespace chess