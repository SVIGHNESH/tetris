#pragma once

#include <array>
#include <span>

#include "tetris/cell.hpp"
#include "tetris/location.hpp"

namespace tetris {

inline constexpr int kOrientations = 4;
inline constexpr int kCellsPerPiece = 4;
inline constexpr int kMaxLevel = 19;
inline constexpr int kLinesPerLevel = 10;

// Cell offsets from a piece origin, indexed by [type][orientation][cell].
// Each shape lives in the 4x4 box anchored at the origin, exactly as in the
// original C table, so the spawn column arithmetic carries over unchanged.
inline constexpr std::array<
    std::array<std::array<Offset, kCellsPerPiece>, kOrientations>,
    kTetrominoCount>
    kTetrominos = {{
        // I
        {{{{{1, 0}, {1, 1}, {1, 2}, {1, 3}}},
          {{{0, 2}, {1, 2}, {2, 2}, {3, 2}}},
          {{{3, 0}, {3, 1}, {3, 2}, {3, 3}}},
          {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}}},
        // J
        {{{{{0, 0}, {1, 0}, {1, 1}, {1, 2}}},
          {{{0, 1}, {0, 2}, {1, 1}, {2, 1}}},
          {{{1, 0}, {1, 1}, {1, 2}, {2, 2}}},
          {{{0, 1}, {1, 1}, {2, 0}, {2, 1}}}}},
        // L
        {{{{{0, 2}, {1, 0}, {1, 1}, {1, 2}}},
          {{{0, 1}, {1, 1}, {2, 1}, {2, 2}}},
          {{{1, 0}, {1, 1}, {1, 2}, {2, 0}}},
          {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}}}},
        // O
        {{{{{0, 1}, {0, 2}, {1, 1}, {1, 2}}},
          {{{0, 1}, {0, 2}, {1, 1}, {1, 2}}},
          {{{0, 1}, {0, 2}, {1, 1}, {1, 2}}},
          {{{0, 1}, {0, 2}, {1, 1}, {1, 2}}}}},
        // S
        {{{{{0, 1}, {0, 2}, {1, 0}, {1, 1}}},
          {{{0, 1}, {1, 1}, {1, 2}, {2, 2}}},
          {{{1, 1}, {1, 2}, {2, 0}, {2, 1}}},
          {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}}},
        // T
        {{{{{0, 1}, {1, 0}, {1, 1}, {1, 2}}},
          {{{0, 1}, {1, 1}, {1, 2}, {2, 1}}},
          {{{1, 0}, {1, 1}, {1, 2}, {2, 1}}},
          {{{0, 1}, {1, 0}, {1, 1}, {2, 1}}}}},
        // Z
        {{{{{0, 0}, {0, 1}, {1, 1}, {1, 2}}},
          {{{0, 2}, {1, 1}, {1, 2}, {2, 1}}},
          {{{1, 0}, {1, 1}, {2, 1}, {2, 2}}},
          {{{0, 1}, {1, 0}, {1, 1}, {2, 0}}}}},
    }};

// Ticks between gravity steps, indexed by level.
inline constexpr std::array<int, kMaxLevel + 1> kGravity = {
    50, 48, 46, 44, 42, 40, 38, 36, 34, 32,
    30, 28, 26, 24, 22, 20, 16, 12, 8,  4,
};

// Wall kick candidates, tried in order; the first one that fits wins. This
// reproduces the original's "rotate, else shift one column either way"
// behaviour as data rather than control flow, so swapping in full SRS tables
// later is a table edit and nothing more.
inline constexpr std::array<Offset, 3> kKicks = {{{0, 0}, {0, -1}, {0, 1}}};

// The I piece is four wide, so it needs a two column shift to clear a wall.
inline constexpr std::array<Offset, 5> kKicksI = {
    {{0, 0}, {0, -1}, {0, 1}, {0, -2}, {0, 2}}};

constexpr std::span<const Offset> kicks_for(Tetromino type) {
  if (type == Tetromino::I) return kKicksI;
  return kKicks;
}

// Points awarded for clearing n lines at once, before the level multiplier.
inline constexpr std::array<int, kCellsPerPiece + 1> kLineMultiplier = {
    0, 40, 100, 300, 1200};

}  // namespace tetris
