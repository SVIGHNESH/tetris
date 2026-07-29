#pragma once

#include <optional>

#include "tetris/cell.hpp"

namespace tetris::ui {

// RAII around ncurses initialisation.
//
// This is the single biggest robustness win over the original, which called
// endwin() only on the paths it remembered. Any other exit, including an
// uncaught exception or a failed assertion, left the player's terminal in raw
// mode with no cursor and no echo. Here the destructor runs regardless.
//
// The header deliberately does not include <ncurses.h>; that header defines
// macros for names as ordinary as clear, erase, and timeout, and there is no
// reason to inflict them on every translation unit in the UI.
class Terminal {
 public:
  Terminal();
  ~Terminal();

  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;
  Terminal(Terminal&&) = delete;
  Terminal& operator=(Terminal&&) = delete;

  int rows() const;
  int cols() const;

  bool colour() const { return colour_; }

  // The next queued keypress, or nothing when the input buffer is empty.
  std::optional<int> next_key() const;

  // Blocks until a key arrives. Used by the menus, where spinning would be
  // pointless.
  int wait_for_key() const;

  void set_blocking(bool blocking) const;

 private:
  bool colour_ = false;
};

// ncurses colour pair index for a cell, or 0 when the terminal is
// monochrome and the renderer should fall back to characters.
short colour_pair_for(Cell cell);

}  // namespace tetris::ui
