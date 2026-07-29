#include "tetris/board.hpp"

#include <algorithm>
#include <cassert>

namespace tetris {

Board::Board(int rows, int cols)
    : rows_(rows),
      cols_(cols),
      cells_(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols),
             Cell::Empty) {
  assert(rows > 0 && cols > 0);
}

bool Board::in_bounds(int row, int col) const {
  return row >= 0 && row < rows_ && col >= 0 && col < cols_;
}

Cell Board::at(int row, int col) const {
  assert(in_bounds(row, col));
  return cells_[index(row, col)];
}

void Board::set(int row, int col, Cell cell) {
  assert(in_bounds(row, col));
  cells_[index(row, col)] = cell;
}

bool Board::fits(const Piece& piece) const {
  for (const Offset& c : piece.cells()) {
    if (!in_bounds(c.row, c.col)) return false;
    if (!is_empty(at(c.row, c.col))) return false;
  }
  return true;
}

void Board::lock(const Piece& piece) {
  for (const Offset& c : piece.cells()) {
    assert(in_bounds(c.row, c.col));
    set(c.row, c.col, piece.cell());
  }
}

bool Board::row_is_full(int row) const {
  for (int col = 0; col < cols_; ++col) {
    if (is_empty(at(row, col))) return false;
  }
  return true;
}

int Board::clear_full_lines() {
  // Walk bottom to top, copying surviving rows down to a write cursor. Rows
  // above the cursor at the end are the ones that were vacated.
  int write = rows_ - 1;
  int cleared = 0;

  for (int read = rows_ - 1; read >= 0; --read) {
    if (row_is_full(read)) {
      ++cleared;
      continue;
    }
    if (write != read) {
      const auto src = cells_.begin() + static_cast<std::ptrdiff_t>(index(read, 0));
      const auto dst = cells_.begin() + static_cast<std::ptrdiff_t>(index(write, 0));
      std::copy_n(src, cols_, dst);
    }
    --write;
  }

  for (int row = write; row >= 0; --row) {
    const auto start = cells_.begin() + static_cast<std::ptrdiff_t>(index(row, 0));
    std::fill_n(start, cols_, Cell::Empty);
  }

  return cleared;
}

}  // namespace tetris
