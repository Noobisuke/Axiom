#include "move.hpp"
#include "bitboard.hpp"
#include <cctype>

namespace chess {

Move Move::from_uci(const char* uci) {
  Move m;
  if (!uci || uci[0] < 'a' || uci[0] > 'h') return m;
  File from_f = File(uci[0] - 'a');
  Rank from_r = Rank(uci[1] - '1');
  File to_f = File(uci[2] - 'a');
  Rank to_r = Rank(uci[3] - '1');
  m.set_from(make_square(from_f, from_r));
  m.set_to(make_square(to_f, to_r));
  if (uci[4]) {
    switch (uci[4]) {
      case 'n': m.set_promotion(PieceType::Knight); break;
      case 'b': m.set_promotion(PieceType::Bishop); break;
      case 'r': m.set_promotion(PieceType::Rook); break;
      case 'q': m.set_promotion(PieceType::Queen); break;
    }
  }
  return m;
}

} // namespace chess