#include "position.hpp"
#include "search.hpp"
#include "eval.hpp"
#include "uci.hpp"
#include <iostream>
#include <chrono>
#include <vector>

int main() {
  std::cout << "Bench starting..." << std::endl;
  chess::Position pos;
  pos.set_startpos();

  // Standard benchmark positions (from Stockfish bench)
  const char* bench_fens[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
  };

  chess::SearchLimits limits;
  limits.depth = 13;
  chess::SearchInfo info;

  uint64_t total_nodes = 0;
  auto total_start = std::chrono::steady_clock::now();

  for (const char* fen : bench_fens) {
    pos.set_fen(fen);
    info = chess::SearchInfo{};
    chess::search(pos, limits, info);
    total_nodes += info.nodes;
    std::cout << "FEN: " << fen << "  nodes: " << info.nodes << "  time: " << info.time_ms << "ms  nps: " << info.nps << std::endl;
  }

  auto total_end = std::chrono::steady_clock::now();
  double total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();

  std::cout << std::endl << "Total nodes: " << total_nodes << std::endl;
  std::cout << "Total time: " << total_ms << " ms" << std::endl;
  std::cout << "Overall NPS: " << (static_cast<double>(total_nodes) * 1000.0 / total_ms) << std::endl;

  return 0;
}