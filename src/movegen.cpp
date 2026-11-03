#include "movegen.hpp"
#include "position.hpp"
#include "move.hpp"
#include "bitboard.hpp"

namespace chess {

// Stub implementations - will be replaced in Phase 3
void generate_moves(const Position&, MoveList&) {}
void generate_captures(const Position&, MoveList&) {}
void generate_quiet(const Position&, MoveList&) {}

bool make_move(Position&, Move) { return false; }
void unmake_move(Position&, Move) {}

uint64_t perft(Position&, int, bool) { return 0; }

} // namespace chess