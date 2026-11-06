// Self-play test (Phase 4 exit criteria): Axiom plays full games against
// a uniformly random mover. Checks: every move played is legal, games
// terminate, and Axiom beats the random mover by a wide margin.
#include "search.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "eval.hpp"
#include <cstdio>
#include <random>
#include <string>

using namespace chess;

static int failures = 0;
static void check(const char* name, bool ok, const char* detail = "") {
  std::printf("%-56s %s %s\n", name, ok ? "PASS" : "FAIL", detail);
  if (!ok) ++failures;
}

// Play one game. axiom_color plays with search depth `depth`, the other
// side moves uniformly at random. Returns +1 if axiom wins, -1 if it
// loses, 0 for draw. Sets illegal=true if a non-legal move is ever chosen.
static int play_game(Color axiom_color, int depth, std::mt19937& rng,
                     bool& illegal, int& plies) {
  Position pos;
  pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  illegal = false;
  plies = 0;

  while (plies < 400) {
    MoveList list;
    generate_moves(pos, list);
    if (list.empty()) {
      // Checkmate or stalemate.
      return in_check(pos, pos.side_to_move)
                 ? (pos.side_to_move == axiom_color ? -1 : +1)
                 : 0;
    }
    if (is_draw(pos)) return 0;

    Move m;
    if (pos.side_to_move == axiom_color) {
      SearchLimits lim;
      lim.depth = depth;
      lim.quiet = true; // keep test output readable
      SearchInfo info;
      search(pos, lim, info);
      m = info.best_move;
      // Verify the chosen move is in the legal list.
      bool found = false;
      for (size_t i = 0; i < list.size(); ++i)
        if (list[i] == m) { found = true; break; }
      if (!found) { illegal = true; return 0; }
    } else {
      std::uniform_int_distribution<size_t> pick(0, list.size() - 1);
      m = list[pick(rng)];
    }

    Undo undo;
    make_move(pos, m, undo);
    ++plies;
  }
  return 0; // move limit: treat as draw
}

int main() {
  init_attack_tables();
  init_sliding_attacks();
  init_zobrist();

  std::mt19937 rng(12345);

  const int GAMES = 20;
  int wins = 0, losses = 0, draws = 0, illegal_count = 0, total_plies = 0;
  for (int g = 0; g < GAMES; ++g) {
    const Color c = (g % 2 == 0) ? Color::White : Color::Black;
    bool illegal = false;
    int plies = 0;
    const int r = play_game(c, 3, rng, illegal, plies);
    if (illegal) ++illegal_count;
    if (r > 0) ++wins;
    else if (r < 0) ++losses;
    else ++draws;
    total_plies += plies;
  }

  char buf[128];
  std::snprintf(buf, sizeof buf, "games=%d W=%d L=%d D=%d illegal=%d avg_plies=%.0f",
                GAMES, wins, losses, draws, illegal_count,
                static_cast<double>(total_plies) / GAMES);
  check("self-play: no illegal moves in 20 games", illegal_count == 0, buf);
  std::snprintf(buf, sizeof buf, "W=%d L=%d D=%d", wins, losses, draws);
  check("self-play: beats random mover (>=17/20 wins)", wins >= 17, buf);

  std::printf("selfplay_test: %s (%d failures)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures);
  return failures == 0 ? 0 : 1;
}
