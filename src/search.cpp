#include "search.hpp"
#include "position.hpp"
#include "eval.hpp"
#include <chrono>

namespace chess {

void search(Position& pos, const SearchLimits& limits, SearchInfo& info) {
  auto start = std::chrono::steady_clock::now();

  // Placeholder: return a dummy best move and score
  info.depth = 1;
  info.score = evaluate(pos);
  info.nodes = 1;
  info.best_move = Move(); // null move

  auto end = std::chrono::steady_clock::now();
  info.time_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
  if (info.time_ms > 0) info.nps = static_cast<int>((static_cast<uint64_t>(info.nodes) * 1000) / info.time_ms);
}

} // namespace chess