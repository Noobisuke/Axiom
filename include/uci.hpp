#pragma once

#include "position.hpp"
#include "search.hpp"
#include <string>
#include <functional>

namespace chess {

class UCI {
  Position pos_;
  SearchLimits limits_;
  SearchInfo info_;
  bool running_ = false;
  bool stop_requested_ = false;

  // Callbacks for async operation (optional)
  std::function<void(const std::string&)> on_output_;

public:
  UCI() = default;
  void set_output_callback(std::function<void(const std::string&)> cb) { on_output_ = std::move(cb); }

  void run(); // Main loop, blocks until "quit"

  // Commands
  void cmd_uci();
  void cmd_isready();
  void cmd_ucinewgame();
  void cmd_position(const std::string& tokens);
  void cmd_go(const std::string& tokens);
  void cmd_stop();
  void cmd_quit();

  // Helpers
  void send(const std::string& msg);
  void send_bestmove();
  void send_info();

  Position& position() { return pos_; }
  const Position& position() const { return pos_; }
  bool stop_requested() const { return stop_requested_; }
  void request_stop() { stop_requested_ = true; }
};

} // namespace chess