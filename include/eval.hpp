#pragma once

#include "position.hpp"

namespace chess {

// Evaluation returns score in centipawns from White's perspective
int evaluate(const Position& pos);

} // namespace chess