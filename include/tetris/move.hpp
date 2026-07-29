#pragma once

namespace tetris {

// Drop is a hard drop: the piece falls as far as it fits and locks
// immediately, matching the original's TM_DROP.
enum class Move { None, Left, Right, RotateCW, RotateCCW, Drop, Hold };

}  // namespace tetris
