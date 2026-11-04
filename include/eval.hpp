#pragma once

#include "position.hpp"

namespace chess {

// Evaluation returns score in centipawns from the SIDE TO MOVE's
// perspective (negamax convention): positive = side to move is better.
int evaluate(const Position& pos);

} // namespace chess