#pragma once

#include <vector>

#include "tetris/cell.hpp"
#include "tetris/piece.hpp"

namespace tetris {

// Settled cells only. The falling piece is never stamped into the board; it
// is composited at read time by Game::at(). That keeps fits() a pure query
// with no remove-then-restore dance around every collision test.
class Board {
 public:
  Board(int rows, int cols);

  Cell at(int row, int col) const;
  bool in_bounds(int row, int col) const;

  // True when every cell of the piece is in bounds and unoccupied.
  bool fits(const Piece& piece) const;

  void lock(const Piece& piece);
  void set(int row, int col, Cell cell);

  // Removes every full row, shifting the rows above down. Returns how many
  // rows went.
  int clear_full_lines();

  bool row_is_full(int row) const;

  int rows() const { return rows_; }
  int cols() const { return cols_; }
  const std::vector<Cell>& cells() const { return cells_; }

  friend bool operator==(const Board&, const Board&) = default;

 private:
  std::size_t index(int row, int col) const {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) +
           static_cast<std::size_t>(col);
  }

  int rows_;
  int cols_;
  std::vector<Cell> cells_;  // row-major, flat
};

}  // namespace tetris
