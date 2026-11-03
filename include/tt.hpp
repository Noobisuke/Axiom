#pragma once

#include "position.hpp"
#include "move.hpp"
#include <cstdint>
#include <array>
#include <memory>

namespace chess {

struct TTEntry {
  uint64_t key = 0;
  Move best_move = Move();
  int16_t score = 0;
  int8_t depth = 0;
  uint8_t flags = 0; // 0=none, 1=exact, 2=lowerbound, 3=upperbound
  uint8_t generation = 0;
};

class TranspositionTable {
  std::unique_ptr<TTEntry[]> entries_;
  size_t size_ = 0; // power of 2
  size_t mask_ = 0;
  uint8_t generation_ = 0;

public:
  TranspositionTable() = default;
  ~TranspositionTable() = default;

  void resize(size_t mb);
  void clear();
  void new_search();

  TTEntry* probe(uint64_t key);
  void store(uint64_t key, Move best_move, int16_t score, int8_t depth, uint8_t flags);

  size_t size_mb() const { return (size_ * sizeof(TTEntry)) / (1024 * 1024); }
  size_t entries() const { return size_; }
};

} // namespace chess