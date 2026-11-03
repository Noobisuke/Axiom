#include "position.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace chess;

static int failures = 0;
static int passes = 0;

static void check(bool ok, const std::string& what) {
  if (ok) {
    ++passes;
  } else {
    ++failures;
    printf("  FAIL: %s\n", what.c_str());
  }
}

// ---------------------------------------------------------------------
// 1. Round-trip: parse then print must reproduce the input exactly.
// ---------------------------------------------------------------------
static void test_roundtrip() {
  printf("== Round-trip (parse -> to_fen) ==\n");
  const char* fens[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
  };
  for (const char* f : fens) {
    Position pos;
    const std::string err = pos.set_fen(f);
    if (!err.empty()) {
      check(false, std::string("unexpected parse error for '") + f + "': " + err);
      continue;
    }
    const std::string out = pos.to_fen();
    const bool same = (out == f);
    if (!same) {
      printf("  in : %s\n  out: %s\n", f, out.c_str());
    }
    check(same, std::string("round-trip mismatch for '") + f + "'");
  }
}

// ---------------------------------------------------------------------
// 2. Four-field FEN defaults.
// ---------------------------------------------------------------------
static void test_four_field() {
  printf("== Four-field FEN ==\n");
  Position pos;
  const std::string err = pos.set_fen("8/8/8/8/8/8/4K2k/8 w - -");
  check(err.empty(), "4-field FEN should parse, got: " + err);
  if (err.empty()) {
    check(pos.halfmove_clock == 0, "halfmove defaults to 0");
    check(pos.fullmove_number == 1, "fullmove defaults to 1");
  }
}

// ---------------------------------------------------------------------
// 3. Invalid input must be rejected with a non-empty message.
// ---------------------------------------------------------------------
static void test_invalid() {
  printf("== Invalid input rejection ==\n");
  struct Case { const char* fen; const char* label; };
  const Case cases[] = {
    {"",                                                       "empty string"},
    {"   \t  ",                                                "whitespace only"},
    {"8/8/8/8/8/8/8 w - - 0 1",                                "only 7 ranks"},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNRR w KQkq - 0 1", "rank sums to 9"},
    {"8/8/8/8/8/8/8/RNBQKBN w - - 0 1",                        "rank sums to 7"},
    {"8/8/8/8/8/8/8/4x3 w - - 0 1",                            "unknown piece letter"},
    {"4k3/8/8/8/8/8/8/3KK3 w - - 0 1",                         "two white kings"},
    {"8/8/8/8/8/8/8/4K3 w - - 0 1",                            "zero black kings"},
    {"4k3/8/8/8/8/8/8/4K2P w - - 0 1",                         "white pawn on rank 1"},
    {"4k2P/8/8/8/8/8/8/4K3 w - - 0 1",                         "white pawn on rank 8"},
    {"8/8/8/8/8/8/8/4K2k x - - 0 1",                           "side to move 'x'"},
    {"r3k2r/8/8/8/8/8/8/4K2R w KQkqK - 0 1",                   "repeated castling 'K'"},
    {"r3k2r/8/8/8/8/8/8/4K2R w Z - 0 1",                       "castling 'Z'"},
    {"4k3/8/8/8/8/8/8/4K3 w - e9 0 1",                         "en passant 'e9'"},
    {"4k3/8/8/8/4P3/8/8/4K3 w - e3 0 1",                       "en passant wrong rank"},
    {"4k3/8/8/8/8/8/8/4K2r b - - 0 1",                         "side not to move in check"},
    {"8/8/8/8/8/8/8/4K2k w - - -1 1",                          "negative halfmove clock"},
    {"8/8/8/8/8/8/8/4K2k w - - x 1",                           "non-numeric halfmove clock"},
    {"r3k2r/8/8/8/8/8/8/3K3R w KQkq - 0 1",                    "castling K with king off e1"},
  };

  for (const Case& c : cases) {
    Position pos;
    const std::string err = pos.set_fen(c.fen);
    const bool rejected = !err.empty();
    if (!rejected) {
      printf("  NOT REJECTED: %s  ('%s')\n", c.label, c.fen);
    } else {
      printf("  rejected %-32s : %s\n", c.label, err.c_str());
    }
    check(rejected, std::string("should reject: ") + c.label);
  }
}

// ---------------------------------------------------------------------
// 4. Mailbox and bitboards must agree after every successful parse.
// ---------------------------------------------------------------------
static void test_mailbox_consistency() {
  printf("== Mailbox / bitboard consistency ==\n");
  const char* fens[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
  };
  for (const char* f : fens) {
    Position pos;
    const std::string err = pos.set_fen(f);
    if (!err.empty()) {
      check(false, std::string("parse error: ") + err);
      continue;
    }

    // Rebuild from the mailbox and compare against the live bitboards.
    Position rebuilt = pos;
    rebuilt.recompute_from_mailbox();

    bool ok = (rebuilt.occupied == pos.occupied);
    for (int c = 0; c < 2 && ok; ++c)
      for (int pt = 0; pt < 6 && ok; ++pt)
        if (rebuilt.pieces[c][pt] != pos.pieces[c][pt]) ok = false;

    check(ok, std::string("mailbox/bitboards disagree for '") + f + "'");

    // A full validate() pass must also succeed on a position that parsed.
    std::string verr;
    check(pos.validate(verr), std::string("validate() failed for '") + f + "': " + verr);
  }
}

// ---------------------------------------------------------------------
// 5. Zobrist initialisation.
// ---------------------------------------------------------------------
static void test_zobrist() {
  printf("== Zobrist ==\n");

  Position a, b;
  a.set_startpos();
  b.set_startpos();
  check(a.zobrist_key != 0, "startpos key is non-zero");
  check(a.zobrist_key == b.zobrist_key, "two startpos copies hash identically");
  check(a.zobrist_key == compute_key(a), "incremental key equals recomputed key");

  // Keys must differ between distinct positions.
  Position c;
  c.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
  check(c.zobrist_key != a.zobrist_key, "side-to-move changes the key");

  Position d;
  d.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w kq - 0 1");
  check(d.zobrist_key != a.zobrist_key, "castling rights change the key");

  Position e;
  e.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  check(e.zobrist_key == a.zobrist_key, "identical FEN hashes identically");

  // Distinct square/piece/turn combinations should not collide by accident.
  Position f;
  f.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w Qkq - 0 1");
  check(f.zobrist_key != d.zobrist_key, "different castling sets differ");
}

int main() {
  init_attack_tables();
  init_sliding_attacks();
  init_zobrist();

  test_roundtrip();
  test_four_field();
  test_invalid();
  test_mailbox_consistency();
  test_zobrist();

  printf("\n%d passed, %d failed\n", passes, failures);
  return failures == 0 ? 0 : 1;
}
