#pragma once

namespace tetris {

// Signed, and used for both absolute board positions and deltas. A piece
// origin can legitimately sit at a negative column while part of the shape is
// still on the board, so unsigned would be wrong here.
struct Offset {
  int row = 0;
  int col = 0;

  friend constexpr bool operator==(Offset, Offset) = default;

  friend constexpr Offset operator+(Offset a, Offset b) {
    return {a.row + b.row, a.col + b.col};
  }

  friend constexpr Offset operator-(Offset a, Offset b) {
    return {a.row - b.row, a.col - b.col};
  }
};

}  // namespace tetris
