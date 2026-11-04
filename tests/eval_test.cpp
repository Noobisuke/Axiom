#include "eval.hpp"
#include "position.hpp"
#include "bitboard.hpp"
#include <cstdio>
#include <string>

using namespace chess;

static int failures = 0;

static void check(const char* name, bool ok, int got, int want_or_bound) {
  std::printf("%-58s got %6d  %s\n", name, got, ok ? "PASS" : "FAIL");
  if (!ok) ++failures;
}

static int eval_fen(const char* fen) {
  Position pos;
  const std::string err = pos.set_fen(fen);
  if (!err.empty()) {
    std::printf("FEN REJECTED: %s (%s)\n", fen, err.c_str());
    ++failures;
    return 0;
  }
  return evaluate(pos);
}

int main() {
  init_attack_tables();
  init_sliding_attacks();
  init_zobrist();

  // 1. Start position is perfectly symmetric -> 0 for either side to move.
  check("startpos w to move == 0", eval_fen(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") == 0, eval_fen(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), 0);
  check("startpos b to move == 0", eval_fen(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1") == 0, eval_fen(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1"), 0);

  // 2. White up a queen, white to move -> strongly positive (stm perspective).
  const int wq_w = eval_fen("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  check("white up a queen, w to move > 800", wq_w > 800, wq_w, 900);

  // 3. Same position, black to move -> strongly negative (stm flipped).
  const int wq_b = eval_fen("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
  check("white up a queen, b to move < -800", wq_b < -800, wq_b, -900);

  // 4. Mirror image: black up a queen, black to move -> strongly positive.
  const int bq_b = eval_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR b KQkq - 0 1");
  check("black up a queen, b to move > 800", bq_b > 800, bq_b, 900);

  // 5. Color-flip symmetry: a position and its color-swapped mirror with
  //    the SAME side to move must evaluate to exact negations (stm-relative
  //    score). Flipping stm as well would give the same score, not negated.
  const int a = eval_fen("8/8/8/8/8/8/4P3/4K2k w - - 0 1");
  const int b = eval_fen("4k2K/4p3/8/8/8/8/8/8 w - - 0 1");
  check("color-flipped position negates (same stm)", a == -b, a + b, 0);

  // 6. Kiwipete is roughly balanced: eval must be small in magnitude.
  const int kiwi = eval_fen(
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
  check("kiwipete |eval| < 150", kiwi > -150 && kiwi < 150, kiwi, 0);

  // 7. Deterministic: same FEN twice gives the same score.
  check("deterministic", eval_fen(
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1") == kiwi, 0, 0);

  std::printf("eval_test: %s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
  return failures == 0 ? 0 : 1;
}
