#include "eval.hpp"
#include "position.hpp"
#include "bitboard.hpp"

namespace chess {

// Piece-square tables, written from White's perspective with rank 8
// first (index 0 = a8 ... index 63 = h1). A black piece on square s
// uses the entry of the vertically mirrored square (s ^ 56).
// Values are the classic "simplified evaluation" tables (CPW).
static constexpr int PST[6][64] = {
  // Pawn
  {  0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0 },
  // Knight
  { -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50 },
  // Bishop
  { -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20 },
  // Rook
  {  0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0 },
  // Queen
  { -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20 },
  // King (middlegame)
  { -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20 },
};

// PST index for a piece of `c` on square s (A1 = 0). Tables are stored
// rank-8-first (index 0 = a8), so a white piece maps via
// i = (7 - rank) * 8 + file. Black mirrors vertically first: s ^ 56
// flips the rank, then the same formula applies. (Note: 63 - sq would
// be a 180-degree rotation, not a mirror -- it only coincides for
// left-right symmetric tables.)
static inline int pst_index(Color c, Square s) {
  const int sq = static_cast<int>(s);
  const int m = (c == Color::White) ? sq : (sq ^ 56);
  return (7 - (m >> 3)) * 8 + (m & 7);
}

int evaluate(const Position& pos) {
  int score = 0; // accumulate from White's perspective first

  for (int pt = 0; pt < 6; ++pt) {
    Bitboard w = pos.pieces_of(Color::White, PieceType(pt));
    Bitboard b = pos.pieces_of(Color::Black, PieceType(pt));
    while (w) {
      const Square s = pop_lsb(w);
      score += PIECE_VALUES[pt] + PST[pt][pst_index(Color::White, s)];
    }
    while (b) {
      const Square s = pop_lsb(b);
      score -= PIECE_VALUES[pt] + PST[pt][pst_index(Color::Black, s)];
    }
  }

  // Negamax convention: score from the side to move's perspective.
  return (pos.side_to_move == Color::White) ? score : -score;
}

} // namespace chess