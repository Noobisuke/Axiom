#pragma once

#include "position.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include "tt.hpp"
#include <cstdint>

namespace chess {

// Score constants. Mate scores are MATE_SCORE - ply, so a mate in n
// (2n-1 plies from the root) scores MATE_SCORE - (2n-1) at the root and
// the engine prefers faster mates automatically.
constexpr int MATE_SCORE = 30000;
constexpr int INF_SCORE  = 32000;
constexpr int MATE_THRESHOLD = MATE_SCORE - 1000; // |score| above this = mate

inline bool is_mate_score(int score) {
  return score > MATE_THRESHOLD || score < -MATE_THRESHOLD;
}

struct SearchLimits {
  int depth = -1;       // -1 = unlimited (until other limit)
  int nodes = -1;       // -1 = unlimited
  int movetime = -1;    // ms
  int wtime = -1;       // ms
  int btime = -1;
  int winc = 0;
  int binc = 0;
  int movestogo = -1;
  bool infinite = false;
  bool ponder = false;
  bool quiet = false;   // suppress UCI info lines (used by tests/self-play)
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

// Run iterative deepening from depth 1 up to limits.depth (or until a
// node/time limit hits). Fills info and prints UCI "info" lines.
void search(Position& pos, const SearchLimits& limits, SearchInfo& info);

// Exposed for tests: plain negamax with alpha-beta, static eval at the
// horizon, mate-distance scores. No TT (Phase 4e), no draw detection
// (4f) yet -- those are separate steps.
int negamax(Position& pos, int depth, int alpha, int beta, int ply,
            uint64_t& nodes, bool& stopped);

// Quiescence search: stand-pat + legal captures only (Phase 4c).
int qsearch(Position& pos, int alpha, int beta, int ply,
            uint64_t& nodes, bool& stopped);

// MVV-LVA move ordering (Phase 4d): captures first, most valuable victim /
// least valuable attacker first; promotions next; quiet moves last.
void order_moves(const Position& pos, MoveList& list);

// Draw detection (Phase 4f): 50-move rule, threefold repetition,
// insufficient material. NOTE: call only after move generation --
// checkmate takes priority over the 50-move rule.
bool is_draw(const Position& pos);

// Global transposition table (Phase 4e). Sized once (16 MB) on first use.
extern TranspositionTable g_tt;

} // namespace chess