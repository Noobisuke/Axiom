#include "uci.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <iostream>

int main() {
  // One-time initialisation. All three must run before any attack or
  // hash query is issued.
  chess::init_attack_tables();
  chess::init_sliding_attacks();
  chess::init_zobrist();

  chess::UCI uci;
  uci.run();
  return 0;
}