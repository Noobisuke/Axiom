#pragma once

#include "position.hpp"
#include "move.hpp"
#include <cstdint>
#include <string>

namespace chess {

// Everything required to revert a move, captured before it is applied.
struct Undo {
    Piece    captured      = Piece(0);   // piece taken (empty if none)
    Square   captured_sq   = Square::None; // where it stood (differs from to() on en passant)
    uint8_t  castling      = 0;
    Square   ep_square     = Square::None;
    int      halfmove_clock = 0;
    int      history_count = 0;
    uint64_t zobrist_key   = 0;
};

// Generate all LEGAL moves for the side to move.
void generate_moves(Position& pos, MoveList& list);

// Generate all LEGAL captures (incl. en passant and promotion-captures)
// for the side to move. Used by quiescence search.
void generate_captures(Position& pos, MoveList& list);

// Apply / revert a move. make_move assumes the move is pseudo-legal.
// Unmake restores the exact previous state, including the Zobrist key.
void make_move(Position& pos, Move m, Undo& undo);
void unmake_move(Position& pos, Move m, const Undo& undo);

// Perft: node count at the given depth (perft(0) == 1).
uint64_t perft(Position& pos, int depth);

// Per-move node counts, one line per legal move, plus a total.
std::string perft_divide(Position& pos, int depth);

// UCI-style rendering of a move, e.g. "e2e4" or "e7e8q".
std::string move_to_uci(Move m);

} // namespace chess