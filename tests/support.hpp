#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>

#include "tetris/board.hpp"
#include "tetris/game.hpp"

namespace tetris::test {

// The bag decides which piece falls first, so a test that needs a particular
// piece asks for the seed that produces it. Scanning is deterministic and
// costs a handful of microseconds.
inline std::uint32_t seed_with_first_piece(Tetromino type) {
  for (std::uint32_t seed = 1; seed < 10000; ++seed) {
    if (Game(4, 10, seed).falling().type() == type) return seed;
  }
  throw std::runtime_error("no seed produces the requested first piece");
}

inline void fill_row(Board& board, int row, Cell cell = Cell::T) {
  for (int col = 0; col < board.cols(); ++col) board.set(row, col, cell);
}

inline void fill_row_except(Board& board, int row,
                            const std::set<int>& holes,
                            Cell cell = Cell::T) {
  for (int col = 0; col < board.cols(); ++col) {
    if (!holes.contains(col)) board.set(row, col, cell);
  }
}

inline int occupied_count(const Game& game) {
  int count = 0;
  for (int row = 0; row < game.rows(); ++row) {
    for (int col = 0; col < game.cols(); ++col) {
      if (!is_empty(game.at(row, col))) ++count;
    }
  }
  return count;
}

// Runs the game forward with no input.
inline TickResult idle(Game& game, int ticks) {
  TickResult result = TickResult::Continue;
  for (int i = 0; i < ticks; ++i) result = game.tick(Move::None);
  return result;
}

inline void repeat(Game& game, Move move, int times) {
  for (int i = 0; i < times; ++i) game.tick(move);
}

// Builds a board whose only gaps are exactly the cells the first piece will
// occupy when hard dropped onto an empty board. Dropping that piece therefore
// completes, and clears, every row its footprint touches.
struct PrimedBoard {
  Board board;
  int rows_completed = 0;
};

inline PrimedBoard prime_for_first_drop(int rows, int cols,
                                        std::uint32_t seed) {
  const Game probe(rows, cols, seed);
  const auto footprint = probe.landing_position().cells();

  std::set<int> touched_rows;
  for (const Offset& c : footprint) touched_rows.insert(c.row);

  Board board(rows, cols);
  for (int row : touched_rows) {
    std::set<int> holes;
    for (const Offset& c : footprint) {
      if (c.row == row) holes.insert(c.col);
    }
    fill_row_except(board, row, holes);
  }
  return {std::move(board), static_cast<int>(touched_rows.size())};
}

}  // namespace tetris::test
