#include "position.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <cctype>
#include <string>
#include <vector>
#include <sstream>

namespace chess {

// =====================================================================
// Zobrist tables
// =====================================================================
uint64_t ZOBRIST_PIECES[2][6][64];
uint64_t ZOBRIST_CASTLING[16];
uint64_t ZOBRIST_EP[64];
uint64_t ZOBRIST_SIDE;

// splitmix64: deterministic, no <random> dependency, no heap.
static uint64_t sm64(uint64_t& x) {
  x += 0x9E3779B97F4A7C15ULL;
  uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

void init_zobrist() {
  uint64_t seed = 0x0123456789ABCDEFULL;
  for (int c = 0; c < 2; ++c)
    for (int pt = 0; pt < 6; ++pt)
      for (int s = 0; s < 64; ++s)
        ZOBRIST_PIECES[c][pt][s] = sm64(seed);
  for (int i = 0; i < 16; ++i) ZOBRIST_CASTLING[i] = sm64(seed);
  for (int i = 0; i < 64; ++i) ZOBRIST_EP[i] = sm64(seed);
  ZOBRIST_SIDE = sm64(seed);
}

// =====================================================================
// Placement primitives
// =====================================================================
void Position::put_piece(Piece pc, Square sq) {
  if (is_empty(pc)) return;
  const int s = static_cast<int>(sq);
  const int c = static_cast<int>(color_of(pc));
  const int pt = static_cast<int>(type_of(pc));
  pieces[c][pt] |= BB_SQUARES[s];
  by_color[c] |= BB_SQUARES[s];
  occupied |= BB_SQUARES[s];
  mailbox[s] = pc;
}

void Position::remove_piece(Square sq) {
  const int s = static_cast<int>(sq);
  const Piece pc = mailbox[s];
  if (is_empty(pc)) return;
  const int c = static_cast<int>(color_of(pc));
  const int pt = static_cast<int>(type_of(pc));
  pieces[c][pt] &= ~BB_SQUARES[s];
  by_color[c] &= ~BB_SQUARES[s];
  occupied &= ~BB_SQUARES[s];
  mailbox[s] = Piece(0);
}

void Position::move_piece(Square from, Square to) {
  const Piece pc = mailbox[static_cast<int>(from)];
  remove_piece(from);
  remove_piece(to);   // no-op when the destination is empty
  put_piece(pc, to);
}

void Position::recompute_from_mailbox() {
  pieces.fill({});
  by_color.fill(0);
  occupied = 0;
  for (int s = 0; s < 64; ++s) {
    const Piece pc = mailbox[s];
    if (is_empty(pc)) continue;
    const int c = static_cast<int>(color_of(pc));
    const int pt = static_cast<int>(type_of(pc));
    pieces[c][pt] |= BB_SQUARES[s];
    by_color[c] |= BB_SQUARES[s];
    occupied |= BB_SQUARES[s];
  }
}

// =====================================================================
// Hashing
// =====================================================================
uint64_t compute_key(const Position& pos) {
  uint64_t k = 0;
  for (int c = 0; c < 2; ++c) {
    for (int pt = 0; pt < 6; ++pt) {
      Bitboard b = pos.pieces[c][pt];
      while (b) {
        const Square sq = pop_lsb(b);
        k ^= ZOBRIST_PIECES[c][pt][static_cast<int>(sq)];
      }
    }
  }
  if (pos.side_to_move == Color::Black) k ^= ZOBRIST_SIDE;
  k ^= ZOBRIST_CASTLING[static_cast<int>(pos.castling) & 0xF];
  if (pos.ep_square != Square::None) k ^= ZOBRIST_EP[static_cast<int>(pos.ep_square)];
  return k;
}

// =====================================================================
// Attack queries
// =====================================================================
Bitboard attackers_to(const Position& pos, Square sq, Bitboard occ) {
  const int s = static_cast<int>(sq);
  Bitboard att = 0;

  // A white pawn on X attacks sq  <=>  X is a black-pawn-attack of sq.
  att |= PAWN_ATTACKS[static_cast<int>(Color::Black)][s]
         & pos.pieces_of(Color::White, PieceType::Pawn);
  att |= PAWN_ATTACKS[static_cast<int>(Color::White)][s]
         & pos.pieces_of(Color::Black, PieceType::Pawn);

  const Bitboard knights = pos.pieces_of(Color::White, PieceType::Knight)
                         | pos.pieces_of(Color::Black, PieceType::Knight);
  att |= KNIGHT_ATTACKS[s] & knights;

  const Bitboard kings = pos.pieces_of(Color::White, PieceType::King)
                       | pos.pieces_of(Color::Black, PieceType::King);
  att |= KING_ATTACKS[s] & kings;

  const Bitboard diag = pos.pieces_of(Color::White, PieceType::Bishop)
                      | pos.pieces_of(Color::Black, PieceType::Bishop)
                      | pos.pieces_of(Color::White, PieceType::Queen)
                      | pos.pieces_of(Color::Black, PieceType::Queen);
  att |= bishop_attacks(sq, occ) & diag;

  const Bitboard orth = pos.pieces_of(Color::White, PieceType::Rook)
                      | pos.pieces_of(Color::Black, PieceType::Rook)
                      | pos.pieces_of(Color::White, PieceType::Queen)
                      | pos.pieces_of(Color::Black, PieceType::Queen);
  att |= rook_attacks(sq, occ) & orth;

  return att;
}

bool in_check(const Position& pos, Color c) {
  const Bitboard kb = pos.king_bb(c);
  if (!kb) return false; // no king (should not happen in a validated position)
  const Square ks = static_cast<Square>(lsb_index(kb));
  return (attackers_to(pos, ks, pos.occupied) & pos.all_pieces(~c)) != 0;
}

// =====================================================================
// Setup
// =====================================================================
void Position::set_startpos() {
  *this = Position();

  static const char* rows[8] = { // index 0 = rank 8
    "rnbqkbnr",
    "pppppppp",
    "........",
    "........",
    "........",
    "........",
    "PPPPPPPP",
    "RNBQKBNR"
  };
  for (int r = 0; r < 8; ++r) {
    for (int f = 0; f < 8; ++f) {
      const char ch = rows[r][f];
      if (ch == '.') continue;
      put_piece(piece_from_char(ch), make_square(File(f), Rank(7 - r)));
    }
  }

  side_to_move = Color::White;
  castling = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
  ep_square = Square::None;
  halfmove_clock = 0;
  fullmove_number = 1;
  zobrist_key = compute_key(*this);

  history_keys[0] = zobrist_key;
  history_halfmoves[0] = halfmove_clock;
  history_count = 1;
}

// Parse a non-negative integer with an overflow guard. Rejects empty
// strings, signs and any non-digit character.
static bool parse_uint(const std::string& s, int& out) {
  if (s.empty()) return false;
  long long v = 0;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + (c - '0');
    if (v > 2000000000LL) return false;
  }
  out = static_cast<int>(v);
  return true;
}

std::string Position::set_fen(const char* fen) {
  *this = Position();
  if (!fen) return "null FEN";

  std::vector<std::string> tok;
  {
    std::istringstream iss(fen);
    std::string t;
    while (iss >> t) tok.push_back(t);
  }
  if (tok.size() < 4)
    return "expected at least 4 FEN fields, got " + std::to_string(tok.size());
  if (tok.size() > 6)
    return "expected at most 6 FEN fields, got " + std::to_string(tok.size());

  // ---- 1. piece placement ----
  {
    int rank = 7;
    int file = 0;
    for (char c : tok[0]) {
      if (c == '/') {
        if (file != 8)
          return "rank " + std::to_string(rank + 1) + " does not sum to 8 squares";
        --rank;
        file = 0;
        if (rank < 0) return "placement field has more than 8 ranks";
      } else if (c >= '1' && c <= '8') {
        file += c - '0';
        if (file > 8)
          return "rank " + std::to_string(rank + 1) + " sums to more than 8 squares";
      } else {
        const Piece pc = piece_from_char(c);
        if (is_empty(pc))
          return std::string("invalid piece character '") + c + "' in placement field";
        if (file >= 8)
          return "rank " + std::to_string(rank + 1) + " sums to more than 8 squares";
        put_piece(pc, make_square(File(file), Rank(rank)));
        ++file;
      }
    }
    if (rank != 0) return "placement field does not have exactly 8 ranks";
    if (file != 8) return "rank 1 does not sum to 8 squares";
  }

  // ---- 2. side to move ----
  if (tok[1] == "w") side_to_move = Color::White;
  else if (tok[1] == "b") side_to_move = Color::Black;
  else return "invalid side to move '" + tok[1] + "' (expected 'w' or 'b')";

  // ---- 3. castling ----
  {
    uint8_t rights = 0;
    if (tok[2] != "-") {
      if (tok[2].empty()) return "empty castling field";
      for (char c : tok[2]) {
        uint8_t bit = 0;
        switch (c) {
          case 'K': bit = CASTLE_WK; break;
          case 'Q': bit = CASTLE_WQ; break;
          case 'k': bit = CASTLE_BK; break;
          case 'q': bit = CASTLE_BQ; break;
          default: return std::string("invalid castling character '") + c + "'";
        }
        if (rights & bit) return std::string("duplicate castling character '") + c + "'";
        rights |= static_cast<uint8_t>(bit);
      }
    }
    // A right is only meaningful if the king and the matching rook are
    // still on their home squares. Per the Step 2 test list we REJECT the
    // FEN rather than silently dropping the right, so that malformed input
    // is never quietly "fixed".
    auto on = [this](Color c, PieceType pt, Square sq) {
      return (pieces[static_cast<int>(c)][static_cast<int>(pt)]
              & BB_SQUARES[static_cast<int>(sq)]) != 0;
    };
    if ((rights & CASTLE_WK)
        && !(on(Color::White, PieceType::King, Square::E1)
             && on(Color::White, PieceType::Rook, Square::H1)))
      return "castling right 'K' but the white king/rook are not on e1/h1";
    if ((rights & CASTLE_WQ)
        && !(on(Color::White, PieceType::King, Square::E1)
             && on(Color::White, PieceType::Rook, Square::A1)))
      return "castling right 'Q' but the white king/rook are not on e1/a1";
    if ((rights & CASTLE_BK)
        && !(on(Color::Black, PieceType::King, Square::E8)
             && on(Color::Black, PieceType::Rook, Square::H8)))
      return "castling right 'k' but the black king/rook are not on e8/h8";
    if ((rights & CASTLE_BQ)
        && !(on(Color::Black, PieceType::King, Square::E8)
             && on(Color::Black, PieceType::Rook, Square::A8)))
      return "castling right 'q' but the black king/rook are not on e8/a8";
    castling = rights;
  }

  // ---- 4. en passant ----
  if (tok[3] != "-") {
    const std::string& es = tok[3];
    if (es.size() != 2) return "invalid en passant square '" + es + "'";
    if (es[0] < 'a' || es[0] > 'h') return "invalid en passant file in '" + es + "'";
    if (es[1] < '1' || es[1] > '8') return "invalid en passant rank in '" + es + "'";
    const Square ep = make_square(File(es[0] - 'a'), Rank(es[1] - '1'));

    const Rank want = (side_to_move == Color::White) ? Rank::Rank6 : Rank::Rank3;
    if (rank_of(ep) != want)
      return "en passant square '" + es + "' is on the wrong rank for the side to move";
    if (occupied & BB_SQUARES[static_cast<int>(ep)])
      return "en passant square '" + es + "' is occupied";

    // The pawn that just double-pushed sits one rank behind the target.
    const int behind = (side_to_move == Color::White) ? -8 : +8;
    const Piece p = mailbox[static_cast<int>(ep) + behind];
    if (is_empty(p) || color_of(p) != ~side_to_move || type_of(p) != PieceType::Pawn)
      return "en passant square '" + es + "' has no enemy pawn to match";

    ep_square = ep;
  }

  // ---- 5/6. clocks ----
  if (tok.size() >= 5) {
    if (!parse_uint(tok[4], halfmove_clock))
      return "invalid halfmove clock '" + tok[4] + "'";
  }
  if (tok.size() >= 6) {
    if (!parse_uint(tok[5], fullmove_number))
      return "invalid fullmove number '" + tok[5] + "'";
    if (fullmove_number < 1) return "fullmove number must be >= 1";
  }

  zobrist_key = compute_key(*this);

  // Reject positions that could not arise in a legal game (kings, pawns,
  // material counts, side-not-to-move in check, mailbox/bitboard agreement).
  std::string err;
  if (!validate(err)) return err;

  history_keys[0] = zobrist_key;
  history_halfmoves[0] = halfmove_clock;
  history_count = 1;
  return "";
}

// =====================================================================
// Printing
// =====================================================================
std::string Position::to_fen() const {
  std::string out;
  for (int r = 7; r >= 0; --r) {
    int run = 0;
    for (int f = 0; f < 8; ++f) {
      const Piece pc = mailbox[static_cast<int>(make_square(File(f), Rank(r)))];
      if (is_empty(pc)) {
        ++run;
        continue;
      }
      if (run) {
        out += static_cast<char>('0' + run);
        run = 0;
      }
      out += piece_to_char(pc);
    }
    if (run) out += static_cast<char>('0' + run);
    if (r != 0) out += '/';
  }

  out += ' ';
  out += (side_to_move == Color::White) ? 'w' : 'b';

  out += ' ';
  if (castling == 0) {
    out += '-';
  } else {
    if (castling & CASTLE_WK) out += 'K';
    if (castling & CASTLE_WQ) out += 'Q';
    if (castling & CASTLE_BK) out += 'k';
    if (castling & CASTLE_BQ) out += 'q';
  }

  out += ' ';
  if (ep_square == Square::None) {
    out += '-';
  } else {
    out += static_cast<char>('a' + static_cast<int>(file_of(ep_square)));
    out += static_cast<char>('1' + static_cast<int>(rank_of(ep_square)));
  }

  out += ' ';
  out += std::to_string(halfmove_clock);
  out += ' ';
  out += std::to_string(fullmove_number);
  return out;
}

// =====================================================================
// Validation
// =====================================================================
bool Position::validate(std::string& error_out) const {
  // Exactly one king per side.
  if (popcount(pieces_of(Color::White, PieceType::King)) != 1) {
    error_out = "white must have exactly one king";
    return false;
  }
  if (popcount(pieces_of(Color::Black, PieceType::King)) != 1) {
    error_out = "black must have exactly one king";
    return false;
  }

  // Kings not adjacent.
  const Square wk = static_cast<Square>(lsb_index(pieces_of(Color::White, PieceType::King)));
  const Square bk = static_cast<Square>(lsb_index(pieces_of(Color::Black, PieceType::King)));
  if (KING_ATTACKS[static_cast<int>(wk)] & BB_SQUARES[static_cast<int>(bk)]) {
    error_out = "kings are adjacent";
    return false;
  }

  // No pawns on the back ranks.
  const Bitboard all_pawns = pieces_of(Color::White, PieceType::Pawn)
                           | pieces_of(Color::Black, PieceType::Pawn);
  if (all_pawns & (RANKS[0] | RANKS[7])) {
    error_out = "a pawn is on rank 1 or rank 8";
    return false;
  }

  // Material limits: <=8 pawns and <=16 pieces per side. Promoted pieces
  // beyond the original complement are allowed, hence the total cap only.
  for (int c = 0; c < 2; ++c) {
    const char* who = (c == 0) ? "white" : "black";
    if (popcount(pieces[c][static_cast<int>(PieceType::Pawn)]) > 8) {
      error_out = std::string(who) + " has more than 8 pawns";
      return false;
    }
    if (popcount(by_color[c]) > 16) {
      error_out = std::string(who) + " has more than 16 pieces";
      return false;
    }
  }

  // Mailbox and bitboards must agree.
  Position rebuilt = *this;
  rebuilt.recompute_from_mailbox();
  for (int c = 0; c < 2; ++c) {
    for (int pt = 0; pt < 6; ++pt) {
      if (rebuilt.pieces[c][pt] != pieces[c][pt]) {
        error_out = "mailbox and bitboards disagree";
        return false;
      }
    }
  }
  if (rebuilt.occupied != occupied) {
    error_out = "mailbox and bitboards disagree (occupancy)";
    return false;
  }

  // The side that just moved cannot be left in check.
  if (in_check(*this, ~side_to_move)) {
    error_out = "side not to move is in check";
    return false;
  }

  // Castling rights must be backed by the board.
  auto on = [this](int c, PieceType pt, Square sq) {
    return (pieces[c][static_cast<int>(pt)] & BB_SQUARES[static_cast<int>(sq)]) != 0;
  };
  if ((castling & CASTLE_WK) && !(on(0, PieceType::King, Square::E1) && on(0, PieceType::Rook, Square::H1))) {
    error_out = "invalid white kingside castling right";
    return false;
  }
  if ((castling & CASTLE_WQ) && !(on(0, PieceType::King, Square::E1) && on(0, PieceType::Rook, Square::A1))) {
    error_out = "invalid white queenside castling right";
    return false;
  }
  if ((castling & CASTLE_BK) && !(on(1, PieceType::King, Square::E8) && on(1, PieceType::Rook, Square::H8))) {
    error_out = "invalid black kingside castling right";
    return false;
  }
  if ((castling & CASTLE_BQ) && !(on(1, PieceType::King, Square::E8) && on(1, PieceType::Rook, Square::A8))) {
    error_out = "invalid black queenside castling right";
    return false;
  }

  error_out.clear();
  return true;
}

} // namespace chess
