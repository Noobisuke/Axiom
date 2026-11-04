#include "search.hpp"
#include "position.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include <chrono>
#include <cstdio>
#include <algorithm>

namespace chess {

namespace {

// Per-search state shared by search() and negamax().
struct SearchState {
  uint64_t nodes = 0;
  bool stopped = false;
  long long budget_ms = -1;              // -1 = no time limit
  std::chrono::steady_clock::time_point start{};
  int max_nodes = -1;
  Move pv[64][64];                       // triangle PV table
  int pv_len[64] = {};
};

SearchState g;

long long elapsed_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::steady_clock::now() - g.start).count();
}

// Called periodically from negamax: enforce node/time limits.
void check_limits() {
  if (g.stopped) return;
  if (g.max_nodes > 0 && g.nodes >= static_cast<uint64_t>(g.max_nodes)) {
    g.stopped = true;
    return;
  }
  if (g.budget_ms > 0 && elapsed_ms() >= g.budget_ms) g.stopped = true;
}

void print_info(int depth, int seldepth, int score, const Move* pv, int pv_len,
                uint64_t nodes, long long ms) {
  std::printf("info depth %d seldepth %d nodes %llu nps %lld time %lld score ",
              depth, seldepth,
              static_cast<unsigned long long>(nodes),
              ms > 0 ? static_cast<long long>(nodes * 1000 / ms) : 0, ms);
  if (is_mate_score(score)) {
    const int plies = MATE_SCORE - (score > 0 ? score : -score);
    std::printf("mate %d", score > 0 ? (plies + 1) / 2 : -(plies + 1) / 2);
  } else {
    std::printf("cp %d", score);
  }
  std::printf(" pv");
  for (int i = 0; i < pv_len; ++i) std::printf(" %s", move_to_uci(pv[i]).c_str());
  std::printf("\n");
  std::fflush(stdout);
}

} // namespace

int negamax(Position& pos, int depth, int alpha, int beta, int ply,
            uint64_t& nodes, bool& stopped) {
  int a = alpha;
  g.pv_len[ply] = 0;

  if (depth <= 0) {
    nodes = g.nodes; stopped = g.stopped;
    return evaluate(pos); // quiescence search is Phase 4c
  }

  if ((g.nodes & 2047) == 0) {
    check_limits();
    if (g.stopped) { nodes = g.nodes; stopped = true; return 0; }
  }
  ++g.nodes;

  MoveList list;
  generate_moves(pos, list);

  if (list.empty()) {
    nodes = g.nodes; stopped = g.stopped;
    // Checkmate (mated now, at this ply) or stalemate.
    return in_check(pos, pos.side_to_move) ? -MATE_SCORE + ply : 0;
  }

  int best = -INF_SCORE;
  Undo undo;
  for (size_t i = 0; i < list.size(); ++i) {
    const Move m = list[i];
    make_move(pos, m, undo);
    const int score = -negamax(pos, depth - 1, -beta, -a, ply + 1,
                               nodes, stopped);
    unmake_move(pos, m, undo);
    if (g.stopped) { nodes = g.nodes; stopped = true; return 0; }

    if (score > best) {
      best = score;
      if (score > a) {
        a = score;
        // Update PV: this move, then the child's PV.
        g.pv[ply][0] = m;
        for (int j = 0; j < g.pv_len[ply + 1]; ++j)
          g.pv[ply][j + 1] = g.pv[ply + 1][j];
        g.pv_len[ply] = g.pv_len[ply + 1] + 1;
      }
    }
    if (a >= beta) break; // beta cutoff
  }

  nodes = g.nodes;
  stopped = g.stopped;
  return best;
}

void search(Position& pos, const SearchLimits& limits, SearchInfo& info) {
  g = SearchState{};
  g.start = std::chrono::steady_clock::now();
  g.max_nodes = limits.nodes;

  // Time budget: explicit movetime wins; otherwise a simple share of the
  // remaining clock (time/30 + increment). Real time management with
  // instability checks is a Phase 9 topic.
  if (limits.movetime > 0) {
    g.budget_ms = limits.movetime;
  } else {
    const int t = (pos.side_to_move == Color::White) ? limits.wtime : limits.btime;
    const int inc = (pos.side_to_move == Color::White) ? limits.winc : limits.binc;
    if (t > 0) g.budget_ms = t / 30 + inc / 2;
  }
  if (limits.infinite) g.budget_ms = -1;

  info = SearchInfo{};

  MoveList root_moves;
  generate_moves(pos, root_moves);

  // Terminal root: mate or stalemate already.
  if (root_moves.empty()) {
    info.score = in_check(pos, pos.side_to_move) ? -MATE_SCORE : 0;
    info.best_move = Move();
    info.nodes = g.nodes;
    return;
  }

  const int max_depth = (limits.depth > 0) ? limits.depth : 64;

  Move best = root_moves[0];
  int best_score = -INF_SCORE;

  for (int d = 1; d <= max_depth; ++d) {
    int alpha = -INF_SCORE;
    Move iter_best = root_moves[0];
    int iter_score = -INF_SCORE;
    int iter_seldepth = d;

    Undo undo;
    for (size_t i = 0; i < root_moves.size(); ++i) {
      const Move m = root_moves[i];
      make_move(pos, m, undo);
      uint64_t n = 0; bool st = false;
      const int score = -negamax(pos, d - 1, -INF_SCORE, -alpha, 1, n, st);
      unmake_move(pos, m, undo);

      if (g.stopped) break;
      if (score > iter_score) {
        iter_score = score;
        iter_best = m;
      }
      if (score > alpha) {
        alpha = score;
        // Root PV: this move followed by the child's PV (ply 1).
        g.pv[0][0] = m;
        for (int j = 0; j < g.pv_len[1]; ++j)
          g.pv[0][j + 1] = g.pv[1][j];
        g.pv_len[0] = g.pv_len[1] + 1;
      }
    }

    if (g.stopped && d > 1) break; // discard the partial iteration
    if (g.stopped && d == 1) {
      // Nothing completed; keep whatever we have.
      if (iter_score > -INF_SCORE) { best = iter_best; best_score = iter_score; }
      break;
    }

    best = iter_best;
    best_score = iter_score;
    info.depth = d;
    info.seldepth = iter_seldepth;

    const long long ms = elapsed_ms();
    print_info(d, iter_seldepth, iter_score, g.pv[0], g.pv_len[0], g.nodes, ms);

    // Early exit on a found mate (mate in n needs depth >= 2n-1).
    if (is_mate_score(iter_score)) {
      const int plies = MATE_SCORE - std::abs(iter_score);
      if (d >= plies) break;
    }
    if (g.budget_ms > 0 && ms >= g.budget_ms) break;
    if (g.max_nodes > 0 && g.nodes >= static_cast<uint64_t>(g.max_nodes)) break;
  }

  const long long ms = elapsed_ms();
  info.nodes = g.nodes;
  info.score = best_score;
  info.best_move = best;
  info.time_ms = static_cast<int>(ms);
  info.nps = ms > 0 ? static_cast<int>(g.nodes * 1000 / ms) : 0;
  if (g.pv_len[0] > 1) info.ponder_move = g.pv[0][1];
}

} // namespace chess