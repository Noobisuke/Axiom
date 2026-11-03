#pragma once

#include "bitboard.hpp"
#include <cstdint>

namespace chess {

// Piece encoding:
// 0 = empty
// 1-6  = White Pawn, Knight, Bishop, Rook, Queen, King
// 9-14 = Black Pawn, Knight, Bishop, Rook, Queen, King
// (gap 7-8 unused for easier color separation)
struct Piece {
    uint8_t v = 0;
    constexpr Piece() = default;
    constexpr explicit Piece(uint8_t v_) : v(v_) {}
    constexpr bool operator==(Piece other) const { return v == other.v; }
    constexpr bool operator!=(Piece other) const { return v != other.v; }
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece(static_cast<uint8_t>(static_cast<int>(pt) + 1 + static_cast<int>(c) * 8));
}

constexpr Color color_of(Piece p) {
    return Color((p.v - 1) >> 3);
}

constexpr PieceType type_of(Piece p) {
    return PieceType((p.v - 1) & 7);
}

constexpr bool is_empty(Piece p) {
    return p.v == 0;
}

constexpr Piece piece_from_char(char c) {
    Color col = (c >= 'a') ? Color::Black : Color::White;
    PieceType pt;
    switch (c & ~0x20) { // uppercase
        case 'P': pt = PieceType::Pawn; break;
        case 'N': pt = PieceType::Knight; break;
        case 'B': pt = PieceType::Bishop; break;
        case 'R': pt = PieceType::Rook; break;
        case 'Q': pt = PieceType::Queen; break;
        case 'K': pt = PieceType::King; break;
        default: return Piece(0);
    }
    return make_piece(col, pt);
}

constexpr char piece_to_char(Piece p) {
    if (is_empty(p)) return '.';
    const char* chars = " PNBRQK  pnbrqk";
    return chars[p.v];
}

// Castling rights bits
constexpr uint8_t CASTLE_WK = 1; // White Kingside
constexpr uint8_t CASTLE_WQ = 2; // White Queenside
constexpr uint8_t CASTLE_BK = 4; // Black Kingside
constexpr uint8_t CASTLE_BQ = 8; // Black Queenside

// Zobrist keys (initialized at startup)
extern uint64_t ZOBRIST_PIECES[2][6][64];
extern uint64_t ZOBRIST_CASTLING[16];
extern uint64_t ZOBRIST_EP[64];
extern uint64_t ZOBRIST_SIDE;

void init_zobrist();

} // namespace chess