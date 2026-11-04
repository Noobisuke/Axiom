#include "search.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include <cstdio>
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

  std::printf("search_test: %s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
  return failures == 0 ? 0 : 1;
}
