#pragma once

#include "bitboard.hpp"
#include "types.hpp"
#include <array>
#include <string>

namespace chess {

struct Position {
    // ---- Piece placement (two redundant representations, kept in sync) ----
    // Bitboards indexed [color][piece_type]; source of truth for movegen.
    std::array<std::array<Bitboard, PIECE_TYPES>, COLORS> pieces{};
    // Occupancy per colour.
    std::array<Bitboard, COLORS> by_color{};
    // Union of both colours.
    Bitboard occupied = 0;
    // Mailbox: what sits on each square. O(1) lookup, no popcount needed.
    std::array<Piece, 64> mailbox{};

    // ---- Game state ----
    Color side_to_move = Color::White;
    // Castling rights, bit 0 = WK, 1 = WQ, 2 = BK, 3 = BQ.
    uint8_t castling = 0;
    // En passant target square, or Square::None.
    Square ep_square = Square::None;
    // Halfmove clock for the 50-move rule.
    int halfmove_clock = 0;
    // Fullmove number (increments after Black's move).
    int fullmove_number = 1;
    // Zobrist hash of this position.
    uint64_t zobrist_key = 0;

    // ---- Repetition history ----
    // Hash + halfmove clock of positions already played, so threefold
    // repetition can be detected later. The repetition *logic* is not
    // implemented yet; the storage exists so make_move can fill it.
    static constexpr int MAX_HISTORY = 1024;
    std::array<uint64_t, MAX_HISTORY> history_keys{};
    std::array<int, MAX_HISTORY> history_halfmoves{};
    int history_count = 0;

    // ---- Setup ----
    void set_startpos();

    // Parse a FEN. Returns an empty string on success, otherwise a
    // human-readable error. Accepts 4- or 6-field FENs; missing clocks
    // default to 0 and 1.
    std::string set_fen(const char* fen);

    // Render the position as a standard 6-field FEN.
    std::string to_fen() const;

    // Sanity-check the position. Returns true when legal, else fills
    // error_out with the reason.
    bool validate(std::string& error_out) const;

    // Rebuild all bitboards and occupancy from the mailbox. Used to prove
    // the two representations agree.
    void recompute_from_mailbox();

    // ---- Placement primitives (keep mailbox and bitboards in sync) ----
    void put_piece(Piece pc, Square sq);
    void remove_piece(Square sq);
    void move_piece(Square from, Square to);

    // ---- Queries ----
    constexpr Bitboard pieces_of(Color c, PieceType pt) const {
        return pieces[static_cast<int>(c)][static_cast<int>(pt)];
    }
    constexpr Bitboard all_pieces(Color c) const {
        return by_color[static_cast<int>(c)];
    }
    constexpr Piece piece_on(Square sq) const {
        return mailbox[static_cast<int>(sq)];
    }
    Bitboard king_bb(Color c) const {
        return pieces[static_cast<int>(c)][static_cast<int>(PieceType::King)];
    }
};

// ---- Free functions ----

// Squares that attack `sq`, given an occupancy. Used for check detection.
Bitboard attackers_to(const Position& pos, Square sq, Bitboard occ);

// Is colour `c`'s king attacked?
bool in_check(const Position& pos, Color c);

// Recompute the Zobrist key from scratch. Used after FEN/startpos setup
// and as a debug cross-check against the incremental key.
uint64_t compute_key(const Position& pos);

} // namespace chess