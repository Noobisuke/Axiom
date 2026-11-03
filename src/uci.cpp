#include "uci.hpp"
#include "position.hpp"
#include "search.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

namespace chess {

void UCI::send(const std::string& msg) {
  if (on_output_) on_output_(msg);
  else std::cout << msg << std::endl;
}

void UCI::cmd_uci() {
  send("id name Axiom 0.1");
  send("id author YourName");
  send("option name Hash type spin default 16 min 1 max 1024");
  send("option name Threads type spin default 1 min 1 max 12");
  send("uciok");
}

void UCI::cmd_isready() {
  send("readyok");
}

void UCI::cmd_ucinewgame() {
  pos_.set_startpos();
}

void UCI::cmd_position(const std::string& tokens) {
  std::istringstream iss(tokens);
  std::string word;
  iss >> word;

  if (word == "startpos") {
    pos_.set_startpos();
    iss >> word; // "moves" or nothing
  } else if (word == "fen") {
    std::string fen;
    for (int i = 0; i < 6 && iss >> word; ++i) {
      if (i) fen += ' ';
      fen += word;
    }
    pos_.set_fen(fen.c_str());
    iss >> word; // "moves" or nothing
  }

  if (word == "moves") {
    Undo undo;
    while (iss >> word) {
      const Move m = Move::from_uci(word.c_str());
      if (m.data) make_move(pos_, m, undo);
    }
  }
}

void UCI::cmd_go(const std::string& tokens) {
  std::istringstream iss(tokens);
  std::string word;
  limits_ = SearchLimits();

  while (iss >> word) {
    if (word == "depth") iss >> limits_.depth;
    else if (word == "nodes") iss >> limits_.nodes;
    else if (word == "movetime") iss >> limits_.movetime;
    else if (word == "wtime") iss >> limits_.wtime;
    else if (word == "btime") iss >> limits_.btime;
    else if (word == "winc") iss >> limits_.winc;
    else if (word == "binc") iss >> limits_.binc;
    else if (word == "movestogo") iss >> limits_.movestogo;
    else if (word == "infinite") limits_.infinite = true;
    else if (word == "ponder") limits_.ponder = true;
  }

  stop_requested_ = false;
  running_ = true;

  // Synchronous search for now
  search(pos_, limits_, info_);

  send_bestmove();
  running_ = false;
}

void UCI::cmd_stop() {
  stop_requested_ = true;
}

void UCI::cmd_quit() {
  // Handled in run()
}

void UCI::send_bestmove() {
  std::ostringstream oss;
  oss << "bestmove ";
  if (info_.best_move.data) {
    Square from = info_.best_move.from();
    Square to = info_.best_move.to();
    oss << char('a' + static_cast<int>(file_of(from))) << char('1' + static_cast<int>(rank_of(from)))
        << char('a' + static_cast<int>(file_of(to))) << char('1' + static_cast<int>(rank_of(to)));
    if (info_.best_move.is_promotion()) {
      char promo = " nbrq"[static_cast<int>(info_.best_move.promotion())];
      oss << promo;
    }
  } else {
    oss << "0000";
  }
  if (info_.ponder_move.data) {
    Square from = info_.ponder_move.from();
    Square to = info_.ponder_move.to();
    oss << " ponder "
        << char('a' + static_cast<int>(file_of(from))) << char('1' + static_cast<int>(rank_of(from)))
        << char('a' + static_cast<int>(file_of(to))) << char('1' + static_cast<int>(rank_of(to)));
  }
  send(oss.str());
}

void UCI::send_info() {
  // Not used in synchronous version
}

void UCI::run() {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "uci") cmd_uci();
    else if (cmd == "isready") cmd_isready();
    else if (cmd == "ucinewgame") cmd_ucinewgame();
    else if (cmd == "position") {
      std::string rest;
      std::getline(iss, rest);
      cmd_position(rest);
    }
    else if (cmd == "go") {
      std::string rest;
      std::getline(iss, rest);
      cmd_go(rest);
    }
    else if (cmd == "stop") cmd_stop();
    else if (cmd == "quit") break;
    // Ignore unknown commands
  }
}

} // namespace chess