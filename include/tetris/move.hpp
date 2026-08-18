#pragma once

namespace tetris {

// Drop is a hard drop: the piece falls as far as it fits and locks
// immediately, matching the original's TM_DROP. SoftDrop is one row, and the
// piece keeps falling.
enum class Move { None, Left, Right, RotateCW, RotateCCW, Drop, SoftDrop, Hold };

}  // namespace tetris
