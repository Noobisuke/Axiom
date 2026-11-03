#include "movegen.hpp"
#include "position.hpp"
#include "move.hpp"
#include "bitboard.hpp"
#include "types.hpp"
#include <sstream>
#include <string>

namespace chess {

// =====================================================================
// Small helpers
// =====================================================================

static inline Move mk(Square from, Square to) {
  Move m;
  m.set_from(from);
  m.set_to(to);
  return m;
}

static void add_promotions(MoveList& list, Square from, Square to) {
  const PieceType promo[4] = {PieceType::Knight, PieceType::Bishop,
                              PieceType::Rook,   PieceType::Queen};
  for (PieceType pt : promo) {
    Move m;
    m.set_from(from);
    m.set_to(to);
    m.set_promotion(pt);
    list.add(m);
  }
}

std::string move_to_uci(Move m) {
  if (!m.data) return "0000";
  std::string s;
  s += static_cast<char>('a' + static_cast<int>(file_of(m.from())));
  s += static_cast<char>('1' + static_cast<int>(rank_of(m.from())));
  s += static_cast<char>('a' + static_cast<int>(file_of(m.to())));
  s += static_cast<char>('1' + static_cast<int>(rank_of(m.to())));
  if (m.is_promotion()) {
    const char ch[6] = {' ', 'n', 'b', 'r', 'q', ' '};
    s += ch[static_cast<int>(m.promotion())];
  }
  return s;
}

// =====================================================================
// Pseudo-legal generation
// =====================================================================

static void generate_pseudo(const Position& pos, MoveList& list) {
  const Color us = pos.side_to_move;
  const Color them = ~us;
  const Bitboard ours = pos.all_pieces(us);
  const Bitboard theirs = pos.all_pieces(them);
  const Bitboard occ = pos.occupied;

  const Bitboard promo_rank = (us == Color::White) ? RANKS[7] : RANKS[0];
  const int up = (us == Color::White) ? +8 : -8;

  // ---------------- Pawns ----------------
  {
    Bitboard pb = pos.pieces_of(us, PieceType::Pawn);

    // Captures (including en passant handled separately below).
    Bitboard caps = pb;
    while (caps) {
      const Square from = pop_lsb(caps);
      Bitboard t = PAWN_ATTACKS[static_cast<int>(us)][static_cast<int>(from)] & theirs;
      while (t) {
        const Square to = pop_lsb(t);
        if (BB_SQUARES[static_cast<int>(to)] & promo_rank)
          add_promotions(list, from, to);
        else
          list.add(mk(from, to));
      }
    }

    // Quiet pushes and promotions by push.
    Bitboard pp = pb;
    while (pp) {
      const Square from = pop_lsb(pp);
      const Square one = static_cast<Square>(static_cast<int>(from) + up);
      if (occ & BB_SQUARES[static_cast<int>(one)]) continue;

      if (BB_SQUARES[static_cast<int>(one)] & promo_rank) {
        add_promotions(list, from, one);
        continue;
      }
      list.add(mk(from, one));

      // Double push: only from the pawn's home rank, and only if the
      // intermediate square was empty (already checked above).
      const Rank home = (us == Color::White) ? Rank::Rank2 : Rank::Rank7;
      if (rank_of(from) == home) {
        const Square two = static_cast<Square>(static_cast<int>(from) + 2 * up);
        if (!(occ & BB_SQUARES[static_cast<int>(two)]))
          list.add(mk(from, two));
      }
    }

    // En passant.
    if (pos.ep_square != Square::None) {
      Bitboard froms = PAWN_ATTACKS[static_cast<int>(them)][static_cast<int>(pos.ep_square)]
                     & pos.pieces_of(us, PieceType::Pawn);
      while (froms) {
        const Square from = pop_lsb(froms);
        list.add(mk(from, pos.ep_square));
      }
    }
  }

  // ---------------- Knights ----------------
  {
    Bitboard b = pos.pieces_of(us, PieceType::Knight);
    while (b) {
      const Square from = pop_lsb(b);
      Bitboard t = KNIGHT_ATTACKS[static_cast<int>(from)] & ~ours;
      while (t) list.add(mk(from, pop_lsb(t)));
    }
  }

  // ---------------- Bishops / Rooks / Queens ----------------
  {
    Bitboard b = pos.pieces_of(us, PieceType::Bishop);
    while (b) {
      const Square from = pop_lsb(b);
      Bitboard t = bishop_attacks(from, occ) & ~ours;
      while (t) list.add(mk(from, pop_lsb(t)));
    }
  }
  {
    Bitboard b = pos.pieces_of(us, PieceType::Rook);
    while (b) {
      const Square from = pop_lsb(b);
      Bitboard t = rook_attacks(from, occ) & ~ours;
      while (t) list.add(mk(from, pop_lsb(t)));
    }
  }
  {
    Bitboard b = pos.pieces_of(us, PieceType::Queen);
    while (b) {
      const Square from = pop_lsb(b);
      Bitboard t = (bishop_attacks(from, occ) | rook_attacks(from, occ)) & ~ours;
      while (t) list.add(mk(from, pop_lsb(t)));
    }
  }

  // ---------------- King ----------------
  {
    Bitboard b = pos.pieces_of(us, PieceType::King);
    while (b) {
      const Square from = pop_lsb(b);
      Bitboard t = KING_ATTACKS[static_cast<int>(from)] & ~ours;
      while (t) list.add(mk(from, pop_lsb(t)));
    }
  }

  // ---------------- Castling ----------------
  // Fully legality-checked here: rights present, path empty, king not in
  // check and not passing through or landing on an attacked square.
  {
    auto attacked_by = [&](Square sq) {
      return (attackers_to(pos, sq, occ) & theirs) != 0;
    };

    if (us == Color::White) {
      if ((pos.castling & CASTLE_WK)
          && !(occ & (BB_SQUARES[static_cast<int>(Square::F1)]
                      | BB_SQUARES[static_cast<int>(Square::G1)]))
          && !attacked_by(Square::E1)
          && !attacked_by(Square::F1)
          && !attacked_by(Square::G1)) {
        list.add(mk(Square::E1, Square::G1));
      }
      if ((pos.castling & CASTLE_WQ)
          && !(occ & (BB_SQUARES[static_cast<int>(Square::B1)]
                      | BB_SQUARES[static_cast<int>(Square::C1)]
                      | BB_SQUARES[static_cast<int>(Square::D1)]))
          && !attacked_by(Square::E1)
          && !attacked_by(Square::D1)
          && !attacked_by(Square::C1)) {
        list.add(mk(Square::E1, Square::C1));
      }
    } else {
      if ((pos.castling & CASTLE_BK)
          && !(occ & (BB_SQUARES[static_cast<int>(Square::F8)]
                      | BB_SQUARES[static_cast<int>(Square::G8)]))
          && !attacked_by(Square::E8)
          && !attacked_by(Square::F8)
          && !attacked_by(Square::G8)) {
        list.add(mk(Square::E8, Square::G8));
      }
      if ((pos.castling & CASTLE_BQ)
          && !(occ & (BB_SQUARES[static_cast<int>(Square::B8)]
                      | BB_SQUARES[static_cast<int>(Square::C8)]
                      | BB_SQUARES[static_cast<int>(Square::D8)]))
          && !attacked_by(Square::E8)
          && !attacked_by(Square::D8)
          && !attacked_by(Square::C8)) {
        list.add(mk(Square::E8, Square::C8));
      }
    }
  }
}

// =====================================================================
// Legal generation
// =====================================================================

void generate_moves(Position& pos, MoveList& list) {
  list.clear();

  MoveList pseudo;
  generate_pseudo(pos, pseudo);

  Undo undo;
  for (size_t i = 0; i < pseudo.size(); ++i) {
    const Move m = pseudo[i];
    make_move(pos, m, undo);
    // After the move the side to move has flipped, so the mover is
    // ~pos.side_to_move.
    if (!in_check(pos, ~pos.side_to_move)) list.add(m);
    unmake_move(pos, m, undo);
  }
}

// =====================================================================
// Make / unmake
// =====================================================================

void make_move(Position& pos, Move m, Undo& undo) {
  const Square from = m.from();
  const Square to = m.to();
  const Piece moving = pos.piece_on(from);

  // Save everything unmake needs.
  undo.captured = Piece(0);
  undo.captured_sq = Square::None;
  undo.castling = pos.castling;
  undo.ep_square = pos.ep_square;
  undo.halfmove_clock = pos.halfmove_clock;
  undo.history_count = pos.history_count;
  undo.zobrist_key = pos.zobrist_key;

  const PieceType pt = type_of(moving);
  const Color us = pos.side_to_move;

  uint64_t k = pos.zobrist_key;

  // Take out the contributions that are about to change.
  if (pos.ep_square != Square::None)
    k ^= ZOBRIST_EP[static_cast<int>(pos.ep_square)];
  k ^= ZOBRIST_CASTLING[static_cast<int>(pos.castling) & 0xF];

  // ---- identify the capture ----
  Piece captured = pos.piece_on(to);
  Square cap_sq = to;
  if (pt == PieceType::Pawn && to == pos.ep_square && is_empty(captured)) {
    const int behind = (us == Color::White) ? -8 : +8;
    cap_sq = static_cast<Square>(static_cast<int>(to) + behind);
    captured = pos.piece_on(cap_sq);
  }
  undo.captured = captured;
  undo.captured_sq = cap_sq;

  if (!is_empty(captured)) {
    pos.remove_piece(cap_sq);
    k ^= ZOBRIST_PIECES[static_cast<int>(color_of(captured))]
                        [static_cast<int>(type_of(captured))]
                        [static_cast<int>(cap_sq)];
  }

  // ---- move the piece ----
  pos.remove_piece(from);
  k ^= ZOBRIST_PIECES[static_cast<int>(us)][static_cast<int>(pt)][static_cast<int>(from)];

  if (m.is_promotion()) {
    const PieceType promoted = m.promotion();
    pos.put_piece(make_piece(us, promoted), to);
    k ^= ZOBRIST_PIECES[static_cast<int>(us)][static_cast<int>(promoted)][static_cast<int>(to)];
  } else {
    pos.put_piece(moving, to);
    k ^= ZOBRIST_PIECES[static_cast<int>(us)][static_cast<int>(pt)][static_cast<int>(to)];
  }

  // ---- castling: relocate the rook ----
  if (pt == PieceType::King) {
    const int diff = static_cast<int>(to) - static_cast<int>(from);
    if (diff == 2 || diff == -2) {
      const int rook_from = (diff == 2) ? static_cast<int>(from) + 3
                                        : static_cast<int>(from) - 4;
      const int rook_to = (diff == 2) ? static_cast<int>(from) + 1
                                      : static_cast<int>(from) - 1;
      const Piece rook = pos.piece_on(static_cast<Square>(rook_from));
      pos.remove_piece(static_cast<Square>(rook_from));
      pos.put_piece(rook, static_cast<Square>(rook_to));
      k ^= ZOBRIST_PIECES[static_cast<int>(us)][static_cast<int>(PieceType::Rook)][rook_from];
      k ^= ZOBRIST_PIECES[static_cast<int>(us)][static_cast<int>(PieceType::Rook)][rook_to];
    }
  }

  // ---- castling rights ----
  {
    uint8_t rights = pos.castling;
    if (pt == PieceType::King) {
      rights &= (us == Color::White)
                  ? static_cast<uint8_t>(~(CASTLE_WK | CASTLE_WQ))
                  : static_cast<uint8_t>(~(CASTLE_BK | CASTLE_BQ));
    }
    switch (static_cast<int>(from)) {
      case static_cast<int>(Square::A1): rights &= static_cast<uint8_t>(~CASTLE_WQ); break;
      case static_cast<int>(Square::H1): rights &= static_cast<uint8_t>(~CASTLE_WK); break;
      case static_cast<int>(Square::A8): rights &= static_cast<uint8_t>(~CASTLE_BQ); break;
      case static_cast<int>(Square::H8): rights &= static_cast<uint8_t>(~CASTLE_BK); break;
      default: break;
    }
    switch (static_cast<int>(cap_sq)) {
      case static_cast<int>(Square::A1): rights &= static_cast<uint8_t>(~CASTLE_WQ); break;
      case static_cast<int>(Square::H1): rights &= static_cast<uint8_t>(~CASTLE_WK); break;
      case static_cast<int>(Square::A8): rights &= static_cast<uint8_t>(~CASTLE_BQ); break;
      case static_cast<int>(Square::H8): rights &= static_cast<uint8_t>(~CASTLE_BK); break;
      default: break;
    }
    pos.castling = rights;
    k ^= ZOBRIST_CASTLING[static_cast<int>(rights) & 0xF];
  }

  // ---- en passant target ----
  pos.ep_square = Square::None;
  if (pt == PieceType::Pawn) {
    const int diff = static_cast<int>(to) - static_cast<int>(from);
    if (diff == 16 || diff == -16) {
      pos.ep_square = static_cast<Square>(static_cast<int>(from) + diff / 2);
    }
  }
  if (pos.ep_square != Square::None)
    k ^= ZOBRIST_EP[static_cast<int>(pos.ep_square)];

  // ---- clocks ----
  pos.halfmove_clock = (pt == PieceType::Pawn || !is_empty(captured))
                         ? 0
                         : pos.halfmove_clock + 1;
  if (us == Color::Black) ++pos.fullmove_number;

  // ---- side to move and hash ----
  k ^= ZOBRIST_SIDE;
  pos.side_to_move = ~us;
  pos.zobrist_key = k;

  // ---- history ----
  if (pos.history_count < Position::MAX_HISTORY) {
    pos.history_keys[pos.history_count] = k;
    pos.history_halfmoves[pos.history_count] = pos.halfmove_clock;
    ++pos.history_count;
  }
}

void unmake_move(Position& pos, Move m, const Undo& undo) {
  const Square from = m.from();
  const Square to = m.to();
  const Color mover = ~pos.side_to_move; // side that played this move

  // Restore scalars first; nothing below depends on them.
  pos.side_to_move = mover;
  pos.castling = undo.castling;
  pos.ep_square = undo.ep_square;
  pos.halfmove_clock = undo.halfmove_clock;
  pos.history_count = undo.history_count;
  pos.zobrist_key = undo.zobrist_key;
  if (mover == Color::Black) --pos.fullmove_number;

  // Undo the piece move (reverse a promotion back into a pawn).
  Piece moved = pos.piece_on(to);
  if (m.is_promotion()) moved = make_piece(mover, PieceType::Pawn);
  pos.remove_piece(to);
  pos.put_piece(moved, from);

  // Undo the castling rook relocation.
  if (type_of(moved) == PieceType::King) {
    const int diff = static_cast<int>(to) - static_cast<int>(from);
    if (diff == 2) {
      pos.remove_piece(static_cast<Square>(static_cast<int>(from) + 1));
      pos.put_piece(make_piece(mover, PieceType::Rook),
                    static_cast<Square>(static_cast<int>(from) + 3));
    } else if (diff == -2) {
      pos.remove_piece(static_cast<Square>(static_cast<int>(from) - 1));
      pos.put_piece(make_piece(mover, PieceType::Rook),
                    static_cast<Square>(static_cast<int>(from) - 4));
    }
  }

  // Restore the captured piece last (its square may equal `from`? no, but
  // doing it last keeps the mailbox write ordering simple).
  if (!is_empty(undo.captured)) {
    pos.put_piece(undo.captured, undo.captured_sq);
  }
}

// =====================================================================
// Perft
// =====================================================================

uint64_t perft(Position& pos, int depth) {
  if (depth <= 0) return 1;

  MoveList list;
  generate_moves(pos, list);

  if (depth == 1) return list.size();

  uint64_t nodes = 0;
  Undo undo;
  for (size_t i = 0; i < list.size(); ++i) {
    const Move m = list[i];
    make_move(pos, m, undo);
    nodes += perft(pos, depth - 1);
    unmake_move(pos, m, undo);
  }
  return nodes;
}

std::string perft_divide(Position& pos, int depth) {
  std::ostringstream oss;
  MoveList list;
  generate_moves(pos, list);

  uint64_t total = 0;
  Undo undo;
  for (size_t i = 0; i < list.size(); ++i) {
    const Move m = list[i];
    make_move(pos, m, undo);
    const uint64_t n = (depth <= 1) ? 1 : perft(pos, depth - 1);
    unmake_move(pos, m, undo);
    total += n;
    oss << move_to_uci(m) << ": " << n << '\n';
  }
  oss << "\nTotal: " << total << '\n';
  return oss.str();
}

} // namespace chess
