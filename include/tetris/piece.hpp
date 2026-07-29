#pragma once

#include <array>

#include "tetris/cell.hpp"
#include "tetris/location.hpp"
#include "tetris/tables.hpp"

namespace tetris {

// A falling piece: what it is, how it is turned, and where its origin sits.
//
// Every mutation returns a new Piece rather than changing this one. Callers
// build a candidate, ask the board whether it fits, and only then keep it.
// That removes the whole "undo the move I just made" family of bugs.
class Piece {
 public:
  Piece(Tetromino type, Offset origin, int orientation = 0);

  // Absolute board coordinates of the four occupied cells.
  std::array<Offset, kCellsPerPiece> cells() const;

  Piece rotated(bool clockwise) const;
  Piece translated(Offset delta) const;
  Piece with_origin(Offset origin) const;

  Tetromino type() const { return type_; }
  Cell cell() const { return to_cell(type_); }
  int orientation() const { return orientation_; }
  Offset origin() const { return origin_; }

  friend bool operator==(const Piece&, const Piece&) = default;

 private:
  Tetromino type_;
  int orientation_;
  Offset origin_;
};

}  // namespace tetris
