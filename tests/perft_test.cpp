#include "position.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cstdint>
#include <string>

using namespace chess;

namespace {

struct Case {
  const char* name;
  const char* fen;
  // expected[d] for d = 1..maxdepth
  const uint64_t* expected;
  int maxdepth;
};

// ---- Start position ----
const uint64_t E_START[] = {0, 20, 400, 8902, 197281, 4865609, 119060324};
// ---- Kiwipete ----
const uint64_t E_KIWI[]  = {0, 48, 2039, 97862, 4085603, 193690690};
// ---- Position 3 ----
const uint64_t E_P3[]    = {0, 14, 191, 2812, 43238, 674624, 11030083};
// ---- Position 4 ----
const uint64_t E_P4[]    = {0, 6, 264, 9467, 422333, 15833292};
// ---- Position 5 ----
const uint64_t E_P5[]    = {0, 44, 1486, 62379, 2103487, 89941194};
// ---- Position 6 ----
const uint64_t E_P6[]    = {0, 46, 2079, 89890, 3894594, 164075551};

const Case CASES[] = {
  {"startpos",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",       E_START, 6},
  {"kiwipete",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", E_KIWI, 5},
  {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                        E_P3,    6},
  {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", E_P4,    5},
  {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",        E_P5,    5},
  {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", E_P6, 5},
};
constexpr int NUM_CASES = static_cast<int>(sizeof(CASES) / sizeof(CASES[0]));

} // namespace

int main(int argc, char** argv) {
  init_attack_tables();
  init_sliding_attacks();
  init_zobrist();

  // Optional: perft_test <case-index> divide <depth>
  if (argc == 4 && std::strcmp(argv[2], "divide") == 0) {
    const int idx = std::atoi(argv[1]);
    const int depth = std::atoi(argv[3]);
    if (idx < 0 || idx >= NUM_CASES) {
      printf("case index must be 0..%d\n", NUM_CASES - 1);
      return 2;
    }
    Position pos;
    const std::string err = pos.set_fen(CASES[idx].fen);
    if (!err.empty()) {
      printf("FEN error: %s\n", err.c_str());
      return 2;
    }
    printf("divide %s depth %d\n%s", CASES[idx].name, depth,
           perft_divide(pos, depth).c_str());
    return 0;
  }

  // Default: depths 1..4 everywhere. Pass a depth to go deeper.
  int max_depth = (argc >= 2) ? std::atoi(argv[1]) : 4;
  if (max_depth < 1) max_depth = 1;

  int passed = 0;
  int failed = 0;

  for (int ci = 0; ci < NUM_CASES; ++ci) {
    const Case& c = CASES[ci];
    Position pos;
    const std::string err = pos.set_fen(c.fen);
    if (!err.empty()) {
      printf("%-10s FEN REJECTED: %s\n", c.name, err.c_str());
      ++failed;
      continue;
    }

    printf("%-10s %s\n", c.name, c.fen);
    for (int d = 1; d <= max_depth && d <= c.maxdepth; ++d) {
      const auto t0 = std::chrono::steady_clock::now();
      const uint64_t nodes = perft(pos, d);
      const auto t1 = std::chrono::steady_clock::now();
      const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

      const uint64_t want = c.expected[d];
      const bool ok = (nodes == want);
      if (ok) ++passed; else ++failed;

      const double nps = (ms > 0.0) ? (static_cast<double>(nodes) * 1000.0 / ms) : 0.0;
      printf("  d%d  %12llu  expected %12llu  %s  %8.1f ms  %10.0f nps\n",
             d,
             static_cast<unsigned long long>(nodes),
             static_cast<unsigned long long>(want),
             ok ? "PASS" : "FAIL",
             ms, nps);
    }
    printf("\n");
  }

  printf("perft: %d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}
