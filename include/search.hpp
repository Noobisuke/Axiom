#pragma once

#include "position.hpp"
#include "move.hpp"

namespace chess {

struct SearchLimits {
  int depth = -1;       // -1 = infinite
  int nodes = -1;
  int movetime = -1;    // ms
  int wtime = -1;       // ms
  int btime = -1;
  int winc = 0;
  int binc = 0;
  int movestogo = -1;
  bool infinite = false;
  bool ponder = false;
};

struct SearchInfo {
  uint64_t nodes = 0;
  int seldepth = 0;
  int depth = 0;
  int score = 0;        // centipawns, from side to move perspective
  Move best_move = Move();
  Move ponder_move = Move();
  int time_ms = 0;
  int nps = 0;
};

void search(Position& pos, const SearchLimits& limits, SearchInfo& info);

} // namespace chess