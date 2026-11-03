#include "bitboard.hpp"
#include <string>
#include <sstream>

namespace chess {

std::string print_bb(Bitboard bb) {
  std::ostringstream oss;
  oss << "\n";
  for (int r = 7; r >= 0; --r) {
    oss << (r + 1) << "  ";
    for (int f = 0; f < 8; ++f) {
      Square sq = make_square(File(f), Rank(r));
      oss << ((bb & BB_SQUARES[static_cast<int>(sq)]) ? "1 " : ". ");
    }
    oss << "\n";
  }
  oss << "\n   a b c d e f g h\n";
  return oss.str();
}

} // namespace chess