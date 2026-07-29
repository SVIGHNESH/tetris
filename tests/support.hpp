#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

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

// A placement is a number of clockwise rotations followed by a number of
// sideways steps, then a hard drop.
struct Placement {
  int rotations = 0;
  int steps = 0;  // negative is left
};

inline void apply(Game& game, Placement placement) {
  for (int i = 0; i < placement.rotations; ++i) game.tick(Move::RotateCW);
  const Move direction = placement.steps < 0 ? Move::Left : Move::Right;
  for (int i = 0; i < std::abs(placement.steps); ++i) game.tick(direction);
  game.tick(Move::Drop);
}

// Shape of the settled stack. The falling piece has to be excluded because
// Game::at() composites it in, and the piece that just respawned at the top
// would otherwise read as a twenty row tower.
struct StackShape {
  int aggregate_height = 0;
  int holes = 0;
  int bumpiness = 0;
};

inline StackShape stack_shape(const Game& game) {
  std::set<std::pair<int, int>> falling;
  for (const Offset& c : game.falling().cells()) falling.emplace(c.row, c.col);

  auto settled = [&](int row, int col) {
    return !is_empty(game.at(row, col)) && !falling.contains({row, col});
  };

  StackShape shape;
  std::vector<int> heights;
  heights.reserve(static_cast<std::size_t>(game.cols()));

  for (int col = 0; col < game.cols(); ++col) {
    int top = game.rows();
    for (int row = 0; row < game.rows(); ++row) {
      if (settled(row, col)) {
        top = row;
        break;
      }
    }
    heights.push_back(game.rows() - top);
    shape.aggregate_height += game.rows() - top;
    for (int row = top + 1; row < game.rows(); ++row) {
      if (!settled(row, col)) ++shape.holes;
    }
  }

  for (std::size_t i = 1; i < heights.size(); ++i) {
    shape.bumpiness += std::abs(heights[i] - heights[i - 1]);
  }
  return shape;
}

// Picks a placement by playing each candidate out on a copy of the game. The
// engine is its own lookahead, so this needs no knowledge of the internals.
// The weights are the well-known four-term Tetris heuristic, which is enough
// to keep a test game alive for hundreds of pieces.
inline Placement best_placement(const Game& game) {
  Placement best;
  double best_value = -std::numeric_limits<double>::infinity();

  for (int rotations = 0; rotations < kOrientations; ++rotations) {
    for (int steps = -game.cols(); steps <= game.cols(); ++steps) {
      Game candidate = game;
      const int before = candidate.lines_cleared();
      apply(candidate, {rotations, steps});
      if (candidate.game_over()) continue;

      const StackShape shape = stack_shape(candidate);
      const double value =
          0.760666 * (candidate.lines_cleared() - before) -
          0.510066 * shape.aggregate_height - 0.35663 * shape.holes -
          0.184483 * shape.bumpiness;
      if (value > best_value) {
        best_value = value;
        best = {rotations, steps};
      }
    }
  }
  return best;
}

// Plays greedily, calling `after_drop` once per placement. Stops on game over
// or after `max_drops` pieces.
template <typename F>
int play_greedy(Game& game, int max_drops, F&& after_drop) {
  int drops = 0;
  while (drops < max_drops && !game.game_over()) {
    apply(game, best_placement(game));
    ++drops;
    after_drop(game);
  }
  return drops;
}

}  // namespace tetris::test
