#include "eval.hpp"
#include "position.hpp"
#include "bitboard.hpp"

namespace chess {

// Material values in centipawns
constexpr int PIECE_VALUES[6] = { 100, 320, 330, 500, 900, 20000 };

int evaluate(const Position& pos) {
  int score = 0;

  for (int pt = 0; pt < 6; ++pt) {
    int w = popcount(pos.pieces_of(Color::White, PieceType(pt)));
    int b = popcount(pos.pieces_of(Color::Black, PieceType(pt)));
    score += (w - b) * PIECE_VALUES[pt];
  }

  // Return from White's perspective
  return score;
}

} // namespace chess