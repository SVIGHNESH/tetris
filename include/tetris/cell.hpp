#pragma once

#include <cstdint>

namespace tetris {

// Cell::Empty is deliberately zero so that a default-constructed board is
// empty. The remaining values are the seven tetromino types offset by one,
// which is what makes to_cell() a plain arithmetic conversion.
enum class Cell : std::uint8_t { Empty = 0, I, J, L, O, S, T, Z };

enum class Tetromino : std::uint8_t { I = 0, J, L, O, S, T, Z };

inline constexpr int kTetrominoCount = 7;

constexpr Cell to_cell(Tetromino type) {
  return static_cast<Cell>(static_cast<std::uint8_t>(type) + 1);
}

constexpr bool is_empty(Cell cell) { return cell == Cell::Empty; }

// Debug rendering only; the ncurses front end uses colour instead.
constexpr char to_char(Cell cell) {
  switch (cell) {
    case Cell::Empty: return '.';
    case Cell::I: return 'I';
    case Cell::J: return 'J';
    case Cell::L: return 'L';
    case Cell::O: return 'O';
    case Cell::S: return 'S';
    case Cell::T: return 'T';
    case Cell::Z: return 'Z';
  }
  return '?';
}

}  // namespace tetris
