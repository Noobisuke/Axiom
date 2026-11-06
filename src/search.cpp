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
  Move pv[128][128];                     // triangle PV table (ply can exceed 64 with qsearch)
  int pv_len[128] = {};
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

// =====================================================================
// Global transposition table (Phase 4e)
// =====================================================================

TranspositionTable g_tt;

// =====================================================================
// Draw detection (Phase 4f)
// =====================================================================

bool is_draw(const Position& pos) {
  // 50-move rule. Caller must check for checkmate FIRST: a mate delivered
  // on the 100th halfmove still wins.
  if (pos.halfmove_clock >= 100) return true;

  // Threefold repetition: the current key has occurred 3+ times in the
  // game history (the current position is the last entry).
  int reps = 0;
  for (int i = 0; i < pos.history_count; ++i)
    if (pos.history_keys[i] == pos.zobrist_key) ++reps;
  if (reps >= 3) return true;

  // Insufficient material: no pawns, rooks or queens, and at most one
  // minor piece per side; or exactly one bishop per side on same-coloured
  // squares.
  const Bitboard heavy =
      pos.pieces_of(Color::White, PieceType::Pawn) | pos.pieces_of(Color::Black, PieceType::Pawn) |
      pos.pieces_of(Color::White, PieceType::Rook) | pos.pieces_of(Color::Black, PieceType::Rook) |
      pos.pieces_of(Color::White, PieceType::Queen) | pos.pieces_of(Color::Black, PieceType::Queen);
  if (heavy) return false;

  const Bitboard minors_w = pos.pieces_of(Color::White, PieceType::Bishop) |
                            pos.pieces_of(Color::White, PieceType::Knight);
  const Bitboard minors_b = pos.pieces_of(Color::Black, PieceType::Bishop) |
                            pos.pieces_of(Color::Black, PieceType::Knight);
  const int nw = popcount(minors_w);
  const int nb = popcount(minors_b);
  if (nw <= 1 && nb <= 1) return true;
  if (nw == 1 && nb == 1 &&
      pos.pieces_of(Color::White, PieceType::Bishop) &&
      pos.pieces_of(Color::Black, PieceType::Bishop)) {
    const Square sw = static_cast<Square>(lsb_index(pos.pieces_of(Color::White, PieceType::Bishop)));
    const Square sb = static_cast<Square>(lsb_index(pos.pieces_of(Color::Black, PieceType::Bishop)));
    if (((static_cast<int>(sw) & 7) + (static_cast<int>(sw) >> 3)) % 2 ==
        ((static_cast<int>(sb) & 7) + (static_cast<int>(sb) >> 3)) % 2)
      return true;
  }
  return false;
}

namespace {

// Mate scores are stored relative to the root (MATE_SCORE - ply), so they
// must be re-based when crossing the TT boundary.
int tt_from_score(int s, int ply) {
  if (s > MATE_THRESHOLD) return s + ply;
  if (s < -MATE_THRESHOLD) return s - ply;
  return s;
}
int tt_to_score(int s, int ply) {
  if (s > MATE_THRESHOLD) return s - ply;
  if (s < -MATE_THRESHOLD) return s + ply;
  return s;
}

} // namespace

// Move ordering (Phase 4d): MVV-LVA
// =====================================================================

void order_moves(const Position& pos, MoveList& list) {
  // Score each move; insertion sort descending (lists are small).
  int scores[MAX_MOVES];
  for (size_t i = 0; i < list.size(); ++i) {
    const Move m = list[i];
    const Piece attacker = pos.piece_on(m.from());
    const Piece victim = pos.piece_on(m.to());
    int s = 0;
    if (!is_empty(victim)) {
      // Capture: MVV-LVA. 10x so victim value dominates attacker value.
      s = 10 * PIECE_VALUES[static_cast<int>(type_of(victim))]
              - PIECE_VALUES[static_cast<int>(type_of(attacker))];
    } else if (m.to() == pos.ep_square && type_of(attacker) == PieceType::Pawn) {
      s = 10 * PIECE_VALUES[static_cast<int>(PieceType::Pawn)]; // en passant
    }
    if (m.is_promotion())
      s += PIECE_VALUES[static_cast<int>(m.promotion())] + 10000;
    scores[i] = s;
  }
  for (size_t i = 1; i < list.size(); ++i) {
    const Move m = list[i];
    const int s = scores[i];
    size_t j = i;
    while (j > 0 && scores[j - 1] < s) {
      list[j] = list[j - 1];
      scores[j] = scores[j - 1];
      --j;
    }
    list[j] = m;
    scores[j] = s;
  }
}

// =====================================================================
// Quiescence search (Phase 4c)
// =====================================================================

int qsearch(Position& pos, int alpha, int beta, int ply,
            uint64_t& nodes, bool& stopped) {
  if ((g.nodes & 2047) == 0) {
    check_limits();
    if (g.stopped) { nodes = g.nodes; stopped = true; return 0; }
  }
  ++g.nodes;

  if (ply >= 100) { nodes = g.nodes; stopped = g.stopped; return evaluate(pos); }

  // Stand pat: the side to move can always "do nothing".
  const int stand = evaluate(pos);
  if (stand >= beta) { nodes = g.nodes; stopped = g.stopped; return stand; }
  if (stand > alpha) alpha = stand;

  MoveList caps;
  generate_captures(pos, caps);
  order_moves(pos, caps);

  Undo undo;
  for (size_t i = 0; i < caps.size(); ++i) {
    const Move m = caps[i];
    make_move(pos, m, undo);
    const int score = -qsearch(pos, -beta, -alpha, ply + 1, nodes, stopped);
    unmake_move(pos, m, undo);
    if (g.stopped) { nodes = g.nodes; stopped = true; return 0; }

    if (score >= beta) { nodes = g.nodes; stopped = g.stopped; return score; }
    if (score > alpha) alpha = score;
  }

  nodes = g.nodes;
  stopped = g.stopped;
  return alpha;
}

// =====================================================================
// Negamax
// =====================================================================

int negamax(Position& pos, int depth, int alpha, int beta, int ply,
            uint64_t& nodes, bool& stopped) {
  int a = alpha;
  const int alpha_orig = alpha;
  g.pv_len[ply] = 0;

  if (depth <= 0) {
    // Quiescence search resolves the horizon (Phase 4c).
    return qsearch(pos, a, beta, ply, nodes, stopped);
  }

  if ((g.nodes & 2047) == 0) {
    check_limits();
    if (g.stopped) { nodes = g.nodes; stopped = true; return 0; }
  }
  ++g.nodes;

  // ---- TT probe (Phase 4e) ----
  const uint64_t key = pos.zobrist_key;
  TTEntry* tt = g_tt.probe(key);
  Move tt_move;
  if (tt) {
    tt_move = tt->best_move;
    const int s = tt_to_score(tt->score, ply);
    if (tt->depth >= depth) {
      if (tt->flags == 1) { nodes = g.nodes; stopped = g.stopped; return s; }          // exact
      if (tt->flags == 2 && s >= beta) { nodes = g.nodes; stopped = g.stopped; return s; } // lower
      if (tt->flags == 3 && s <= a) { nodes = g.nodes; stopped = g.stopped; return s; }    // upper
      // Window narrowing is only sound from entries at least as deep as
      // this node -- a shallower bound says nothing about the deeper value.
      if (tt->flags == 2 && s > a) a = s;
      if (tt->flags == 3 && s < beta) beta = s;
    }
  }

  MoveList list;
  generate_moves(pos, list);
  order_moves(pos, list); // Phase 4d

  // TT move first.
  if (tt_move.data != 0) {
    for (size_t i = 0; i < list.size(); ++i) {
      if (list[i].data == tt_move.data) {
        list[i] = list[0];
        list[0] = tt_move;
        break;
      }
    }
  }

  if (list.empty()) {
    nodes = g.nodes; stopped = g.stopped;
    // Checkmate (mated now, at this ply) or stalemate.
    return in_check(pos, pos.side_to_move) ? -MATE_SCORE + ply : 0;
  }

  // Draws (Phase 4f) -- after the mate check, so mate on move 100 wins.
  if (is_draw(pos)) { nodes = g.nodes; stopped = g.stopped; return 0; }

  int best = -INF_SCORE;
  Move best_move;
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
      best_move = m;
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

  // ---- TT store (Phase 4e), depth-preferred replacement ----
  {
    const uint8_t flags =
        (best >= beta) ? 2 : (best <= alpha_orig) ? 3 : 1;
    TTEntry* slot = g_tt.probe(key);
    if (!slot || depth >= slot->depth)
      g_tt.store(key, best_move, static_cast<int16_t>(tt_from_score(best, ply)),
                 static_cast<int8_t>(depth), flags);
  }

  nodes = g.nodes;
  stopped = g.stopped;
  return best;
}

void search(Position& pos, const SearchLimits& limits, SearchInfo& info) {
  g = SearchState{};
  g.start = std::chrono::steady_clock::now();
  g.max_nodes = limits.nodes;

  // TT: size once (16 MB), bump generation each search (Phase 4e).
  if (g_tt.entries() == 0) g_tt.resize(16);
  g_tt.new_search();

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
  order_moves(pos, root_moves); // Phase 4d at the root too

  // Terminal root: mate or stalemate already.
  if (root_moves.empty()) {
    info.score = in_check(pos, pos.side_to_move) ? -MATE_SCORE : 0;
    info.best_move = Move();
    info.nodes = g.nodes;
    return;
  }

  // Drawn root (threefold / 50-move / insufficient material, Phase 4f):
  // still must return a legal move, but the score is 0. Note the mate
  // priority lives one ply down: a child at halfmove 100 that is mated
  // hits the "no legal moves" branch before is_draw, so a mate delivered
  // on the 100th halfmove still wins.
  if (is_draw(pos)) {
    info.best_move = root_moves[0];
    info.score = 0;
    info.nodes = g.nodes;
    info.depth = 0;
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

    // Try the previous iteration's best move first next round.
    if (d > 1) {
      for (size_t i = 0; i < root_moves.size(); ++i) {
        if (root_moves[i].data == best.data) {
          root_moves[i] = root_moves[0];
          root_moves[0] = best;
          break;
        }
      }
    }

    const long long ms = elapsed_ms();
    if (!limits.quiet)
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