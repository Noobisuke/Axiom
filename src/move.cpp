#include "move.hpp"
#include "bitboard.hpp"
#include <cctype>
#include <cstring>

namespace chess {

Move Move::from_uci(const char* uci) {
  Move m;
  if (!uci) return m;
  const size_t len = std::strlen(uci);
  if (len < 4) return m; // need at least from+to squares
  if (uci[0] < 'a' || uci[0] > 'h' || uci[1] < '1' || uci[1] > '8' ||
      uci[2] < 'a' || uci[2] > 'h' || uci[3] < '1' || uci[3] > '8')
    return m;
  File from_f = File(uci[0] - 'a');
  Rank from_r = Rank(uci[1] - '1');
  File to_f = File(uci[2] - 'a');
  Rank to_r = Rank(uci[3] - '1');
  m.set_from(make_square(from_f, from_r));
  m.set_to(make_square(to_f, to_r));
  if (len >= 5) {
    switch (uci[4]) {
      case 'n': m.set_promotion(PieceType::Knight); break;
      case 'b': m.set_promotion(PieceType::Bishop); break;
      case 'r': m.set_promotion(PieceType::Rook); break;
      case 'q': m.set_promotion(PieceType::Queen); break;
      default: break;
    }
  }
  return m;
}

} // namespace chess