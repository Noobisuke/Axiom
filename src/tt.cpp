#include "tt.hpp"
#include <memory>
#include <cstring>

namespace chess {

void TranspositionTable::resize(size_t mb) {
  size_t bytes = mb * 1024 * 1024;
  size_t new_size = bytes / sizeof(TTEntry);
  // Round up to a power of 2. Clamp first: __builtin_clzll(0) and << 64
  // are both undefined behaviour.
  if (new_size < 2) new_size = 2;
  new_size = 1ULL << (64 - __builtin_clzll(new_size - 1));

  entries_ = std::make_unique<TTEntry[]>(new_size);
  size_ = new_size;
  mask_ = new_size - 1;
  clear();
}

void TranspositionTable::clear() {
  if (entries_) {
    for (size_t i = 0; i < size_; ++i) {
      entries_[i] = TTEntry{};
    }
  }
  generation_ = 0;
}

void TranspositionTable::new_search() {
  generation_ = (generation_ + 1) & 0xFF;
}

TTEntry* TranspositionTable::probe(uint64_t key) {
  if (!entries_) return nullptr;
  size_t idx = key & mask_;
  TTEntry* entry = &entries_[idx];
  if (entry->key == key) return entry;
  return nullptr;
}

void TranspositionTable::store(uint64_t key, Move best_move, int16_t score, int8_t depth, uint8_t flags) {
  if (!entries_) return;
  size_t idx = key & mask_;
  TTEntry* entry = &entries_[idx];
  entry->key = key;
  entry->best_move = best_move;
  entry->score = score;
  entry->depth = depth;
  entry->flags = flags;
  entry->generation = generation_;
}

} // namespace chess