#include "position.hpp"
#include "bitboard.hpp"
#include <cctype>
#include <cstring>

namespace chess {

void Position::set_startpos() {
  // Clear everything
  pieces.fill({});
  by_color.fill(0);
  occupied = 0;
  side_to_move = Color::White;
  castling = 0xF; // KQkq
  ep_square = Square::None;
  halfmove_clock = 0;
  fullmove_number = 1;

  // White pieces
  pieces[0][static_cast<int>(PieceType::Pawn)]   = RANKS[1];
  pieces[0][static_cast<int>(PieceType::Knight)] = BB_SQUARES[1] | BB_SQUARES[6];
  pieces[0][static_cast<int>(PieceType::Bishop)] = BB_SQUARES[2] | BB_SQUARES[5];
  pieces[0][static_cast<int>(PieceType::Rook)]   = BB_SQUARES[0] | BB_SQUARES[7];
  pieces[0][static_cast<int>(PieceType::Queen)]  = BB_SQUARES[3];
  pieces[0][static_cast<int>(PieceType::King)]   = BB_SQUARES[4];

  // Black pieces
  pieces[1][static_cast<int>(PieceType::Pawn)]   = RANKS[6];
  pieces[1][static_cast<int>(PieceType::Knight)] = BB_SQUARES[57] | BB_SQUARES[62];
  pieces[1][static_cast<int>(PieceType::Bishop)] = BB_SQUARES[58] | BB_SQUARES[61];
  pieces[1][static_cast<int>(PieceType::Rook)]   = BB_SQUARES[56] | BB_SQUARES[63];
  pieces[1][static_cast<int>(PieceType::Queen)]  = BB_SQUARES[59];
  pieces[1][static_cast<int>(PieceType::King)]   = BB_SQUARES[60];

  by_color[0] = RANKS[0] | RANKS[1];
  by_color[1] = RANKS[7] | RANKS[6];
  occupied = by_color[0] | by_color[1];
}

bool Position::set_fen(const char* fen) {
  // Minimal FEN parser for startpos and basic positions
  // Format: piece_placement side_to_move castling ep halfmove fullmove
  *this = Position(); // reset

  const char* p = fen;
  int rank = 7, file = 0;

  // 1. Piece placement
  while (*p && *p != ' ') {
    char c = *p++;
    if (c == '/') {
      rank--;
      file = 0;
    } else if (c >= '1' && c <= '8') {
      file += c - '0';
    } else {
      Color col = (c >= 'a') ? Color::Black : Color::White;
      PieceType pt;
      switch (std::toupper(c)) {
        case 'P': pt = PieceType::Pawn; break;
        case 'N': pt = PieceType::Knight; break;
        case 'B': pt = PieceType::Bishop; break;
        case 'R': pt = PieceType::Rook; break;
        case 'Q': pt = PieceType::Queen; break;
        case 'K': pt = PieceType::King; break;
        default: return false;
      }
      if (file >= 8 || rank < 0 || rank > 7) return false;
      Square sq = make_square(File(file), Rank(rank));
      pieces[static_cast<int>(col)][static_cast<int>(pt)] |= bb_square(sq);
      by_color[static_cast<int>(col)] |= bb_square(sq);
      occupied |= bb_square(sq);
      file++;
    }
  }
  if (!*p) return false; // no more fields
  p++; // skip space

  // 2. Side to move
  side_to_move = (*p == 'w') ? Color::White : Color::Black;
  p += 2; // skip "w " or "b "

  // 3. Castling
  castling = 0;
  if (*p != '-') {
    while (*p && *p != ' ') {
      switch (*p++) {
        case 'K': castling |= 1; break;
        case 'Q': castling |= 2; break;
        case 'k': castling |= 4; break;
        case 'q': castling |= 8; break;
      }
    }
  } else {
    p++;
  }
  if (!*p) return false;
  p++; // skip space

  // 4. En passant
  if (*p != '-') {
    File f = File(*p++ - 'a');
    Rank r = Rank(*p++ - '1');
    ep_square = make_square(f, r);
  } else {
    ep_square = Square::None;
    p++;
  }
  if (!*p) return false;
  p++; // skip space

  // 5. Halfmove clock
  halfmove_clock = 0;
  while (*p && *p >= '0' && *p <= '9') {
    halfmove_clock = halfmove_clock * 10 + (*p++ - '0');
  }
  if (!*p) return false;
  p++; // skip space

  // 6. Fullmove number
  fullmove_number = 0;
  while (*p && *p >= '0' && *p <= '9') {
    fullmove_number = fullmove_number * 10 + (*p++ - '0');
  }

  return true;
}

} // namespace chess