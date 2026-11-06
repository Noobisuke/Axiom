#pragma once

#include "position.hpp"

namespace chess {

// Material values in centipawns, indexed by PieceType (Pawn..King).
// Shared by eval and by MVV-LVA move ordering in search.
inline constexpr int PIECE_VALUES[6] = { 100, 320, 330, 500, 900, 20000 };

// Evaluation returns score in centipawns from the SIDE TO MOVE's
// perspective (negamax convention): positive = side to move is better.
int evaluate(const Position& pos);

} // namespace chess