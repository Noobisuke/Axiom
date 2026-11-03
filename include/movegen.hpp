#pragma once

#include "position.hpp"
#include "move.hpp"

namespace chess {

void generate_moves(const Position& pos, MoveList& list);
void generate_captures(const Position& pos, MoveList& list);
void generate_quiet(const Position& pos, MoveList& list);

bool make_move(Position& pos, Move m);
void unmake_move(Position& pos, Move m); // For perft only; search uses copy-make

// Perft
uint64_t perft(Position& pos, int depth, bool divide = false);

} // namespace chess