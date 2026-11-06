#include "search.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace chess;

static int failures = 0;

static void check(const char* name, bool ok, const char* detail = "") {
  std::printf("%-56s %s %s\n", name, ok ? "PASS" : "FAIL", detail);
  if (!ok) ++failures;
}

static SearchInfo run(const char* fen, int depth, int nodes = -1) {
  Position pos;
  const std::string err = pos.set_fen(fen);
  if (!err.empty()) {
    std::printf("FEN REJECTED: %s (%s)\n", fen, err.c_str());
    ++failures;
  }
  SearchLimits lim;
  lim.depth = depth;
  lim.nodes = nodes;
  SearchInfo info;
  search(pos, lim, info);
  return info;
}

static bool is_legal(const Position& pos, Move m) {
  Position p = pos;
  MoveList list;
  generate_moves(p, list);
  for (size_t i = 0; i < list.size(); ++i)
    if (list[i] == m) return true;
  return false;
}

int main() {
  init_attack_tables();
  init_sliding_attacks();
  init_zobrist();

  // 1. Mate in 1 (verified vs Stockfish: a1a8, mate 1).
  {
    Position pos; pos.set_fen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    SearchInfo info = run("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 4);
    char buf[32];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("mate in 1: a1a8, score == MATE-1",
          move_to_uci(info.best_move) == "a1a8" && info.score == MATE_SCORE - 1, buf);
  }

  // 2. Mate in 2 (verified vs Stockfish: f6e6, mate 2 -> score MATE-3).
  {
    SearchInfo info = run("5k2/8/5K2/8/8/8/8/6R1 w - - 0 1", 5);
    char buf[32];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("mate in 2: f6e6, score == MATE-3",
          move_to_uci(info.best_move) == "f6e6" && info.score == MATE_SCORE - 3, buf);
  }

  // 3. Mate in 3 (verified vs Stockfish: h1e4, mate 3 -> score MATE-5).
  {
    SearchInfo info = run("8/8/8/8/8/6k1/8/6KQ w - - 0 1", 8);
    char buf[32];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("mate in 3: h1e4, score == MATE-5",
          move_to_uci(info.best_move) == "h1e4" && info.score == MATE_SCORE - 5, buf);
  }

  // 4. Stalemate at the root: no legal moves, score 0, null best move.
  {
    SearchInfo info = run("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", 3);
    char buf[32];
    std::snprintf(buf, sizeof buf, "best.data=%u score=%d", info.best_move.data, info.score);
    check("stalemate root: null move, score 0",
          info.best_move.data == 0 && info.score == 0, buf);
  }

  // 5. Hanging queen: capture it.
  {
    const char* fen = "8/8/8/8/3q4/2P5/8/4K2k w - - 0 1";
    Position pos; pos.set_fen(fen);
    SearchInfo info = run(fen, 4);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("hanging queen: c3d4, score > 0",
          move_to_uci(info.best_move) == "c3d4" && info.score > 0, buf);
  }

  // 6. Best move from startpos is legal and depth limit respected.
  {
    const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos; pos.set_fen(fen);
    SearchInfo info = run(fen, 5);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s depth=%d", move_to_uci(info.best_move).c_str(), info.depth);
    check("startpos depth 5: legal best move, depth==5",
          info.depth == 5 && is_legal(pos, info.best_move), buf);
  }

  // 7. Node limit stops the search (nodes >= limit).
  {
    SearchInfo info = run("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 64, 5000);
    char buf[48];
    std::snprintf(buf, sizeof buf, "nodes=%llu", static_cast<unsigned long long>(info.nodes));
    check("node limit 5000 respected", info.nodes >= 5000, buf);
  }

  // 8. Quiescence (4c): at depth 1 the leaf is a qsearch node. Qxd8+ looks
  // like +900 statically, but qsearch sees Kxd8 recapture -> trade (~0).
  // Without qsearch this position scores ~+900 at depth 1.
  {
    SearchInfo info = run("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1", 1);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("qsearch: defended queen not 'won' (score small at d1)",
          std::abs(info.score) < 150, buf);
  }

  // 9. Quiescence resolves a hanging queen at depth 1 (no recapture).
  {
    SearchInfo info2 = run("4k3/8/8/8/3q4/8/8/3QK3 w - - 0 1", 1);
    char buf[64];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info2.best_move).c_str(), info2.score);
    check("qsearch: undefended queen captured (d1d4, score ~+900)",
          move_to_uci(info2.best_move) == "d1d4" && info2.score > 700, buf);
  }

  // 10. MVV-LVA ordering (4d): in a position with a hanging queen, the
  // capture must be the first move after order_moves.
  {
    Position pos;
    pos.set_fen("8/8/8/8/3q4/2P5/8/4K2k w - - 0 1");
    MoveList list;
    generate_moves(pos, list);
    order_moves(pos, list);
    char buf[32];
    std::snprintf(buf, sizeof buf, "first=%s", move_to_uci(list[0]).c_str());
    check("ordering: MVV-LVA puts c3d4 (queen capture) first",
          move_to_uci(list[0]) == "c3d4", buf);
  }

  // 11. generate_captures yields only captures.
  {
    Position pos;
    pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    MoveList caps;
    generate_captures(pos, caps);
    bool all_caps = true;
    for (size_t i = 0; i < caps.size(); ++i) {
      const Move m = caps[i];
      if (is_empty(pos.piece_on(m.to())) &&
          !(m.to() == pos.ep_square && type_of(pos.piece_on(m.from())) == PieceType::Pawn))
        all_caps = false;
    }
    char buf[32];
    std::snprintf(buf, sizeof buf, "caps=%zu", caps.size());
    check("generate_captures: startpos has 0 captures", caps.size() == 0, buf);

    pos.set_fen("8/8/8/8/3q4/2P5/8/4K2k w - - 0 1");
    generate_captures(pos, caps);
    bool found = false;
    for (size_t i = 0; i < caps.size(); ++i)
      if (move_to_uci(caps[i]) == "c3d4") found = true;
    std::snprintf(buf, sizeof buf, "caps=%zu found=%d", caps.size(), (int)found);
    check("generate_captures: finds c3d4, only captures", found && all_caps, buf);
  }

  // 12. TT (4e): a repeated identical search must use fewer nodes the
  // second time (entries persist across search() calls).
  {
    g_tt.clear();
    SearchInfo i1 = run("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7);
    SearchInfo i2 = run("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7);
    char buf[96];
    std::snprintf(buf, sizeof buf, "first=%llu second=%llu best=%s",
                  static_cast<unsigned long long>(i1.nodes),
                  static_cast<unsigned long long>(i2.nodes),
                  move_to_uci(i2.best_move).c_str());
    check("TT: second identical search cheaper, same best move",
          i2.nodes < i1.nodes && move_to_uci(i1.best_move) == move_to_uci(i2.best_move), buf);
  }

  // 13. TT keeps mate scores exact (mate in 2 still MATE-3 with TT active).
  {
    g_tt.clear();
    SearchInfo info = run("5k2/8/5K2/8/8/8/8/6R1 w - - 0 1", 5);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("TT: mate in 2 score still MATE-3",
          move_to_uci(info.best_move) == "f6e6" && info.score == MATE_SCORE - 3, buf);
  }

  // 14. Insufficient material (4f): K+N vs K is a dead draw.
  {
    SearchInfo info = run("8/8/8/4k3/8/8/8/4K2N w - - 0 1", 4);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("draw: K+N vs K scores 0", info.score == 0, buf);
  }

  // 15. 50-move rule (4f): halfmove clock 100, no pawns and no captures
  // available (rook-only endgame, so clock can only rise).
  {
    SearchInfo info = run("4k3/8/8/8/8/8/8/R3K3 w - - 100 60", 3);
    char buf[48];
    std::snprintf(buf, sizeof buf, "score=%d", info.score);
    check("draw: halfmove clock 100 scores 0", info.score == 0, buf);
  }

  // 16. Mate delivered ON the 100th halfmove still wins (4f edge case):
  // clock 99 at root, a1a8 is the 100th halfmove and mates.
  {
    SearchInfo info = run("6k1/5ppp/8/8/8/8/8/R5K1 w - - 99 60", 2);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("mate on the 100th halfmove beats the 50-move rule",
          move_to_uci(info.best_move) == "a1a8" && info.score == MATE_SCORE - 1, buf);
  }

  // 17. Threefold repetition (4f): shuffle knights until startpos occurs
  // a third time, then the root must score 0.
  {
    Position pos;
    pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const char* seq[] = {"g1f3", "g8f6", "f3g1", "f6g8",
                         "g1f3", "g8f6", "f3g1", "f6g8"};
    Undo undo;
    for (const char* u : seq) make_move(pos, Move::from_uci(u), undo);
    SearchLimits lim; lim.depth = 4;
    SearchInfo info;
    search(pos, lim, info);
    char buf[48];
    std::snprintf(buf, sizeof buf, "best=%s score=%d", move_to_uci(info.best_move).c_str(), info.score);
    check("draw: threefold repetition scores 0", info.score == 0, buf);
  }

  std::printf("search_test: %s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
  return failures == 0 ? 0 : 1;
}