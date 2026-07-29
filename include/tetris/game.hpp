#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "tetris/board.hpp"
#include "tetris/move.hpp"
#include "tetris/piece.hpp"

namespace tetris {

// An enum rather than a bool so that Locked or LevelUp can be added later and
// the UI can react with a sound or a flash without the engine learning that
// either exists.
enum class TickResult { Continue, GameOver };

class Game {
 public:
  // A 20 row field is the standard, and it is what lets the whole UI, hint
  // line included, fit an 80x24 terminal.
  static constexpr int kDefaultRows = 20;
  static constexpr int kDefaultCols = 10;

  Game(int rows, int cols, std::uint32_t seed);

  // Starts from an existing position rather than an empty board. The save
  // format is one caller, tests that need a specific stack are another, and a
  // puzzle mode would be a third.
  Game(Board board, std::uint32_t seed);

  // Advances one frame: apply the move, run gravity, clear lines, score.
  TickResult tick(Move move);

  // The board with the falling piece composited in, which is what a renderer
  // wants and what the original exposed via tg_get.
  Cell at(int row, int col) const;
  bool in_bounds(int row, int col) const { return board_.in_bounds(row, col); }

  int rows() const { return board_.rows(); }
  int cols() const { return board_.cols(); }
  int score() const { return points_; }
  int level() const { return level_; }
  int lines_remaining() const { return lines_remaining_; }
  int lines_cleared() const { return lines_cleared_; }
  bool game_over() const { return game_over_; }

  const Piece& falling() const { return falling_; }
  const Piece& next() const { return next_; }
  const std::optional<Piece>& held() const { return held_; }

  // False once hold has been used for the current piece, and true again after
  // it locks. The renderer surfaces this so the lockout is visible rather
  // than mysterious.
  bool can_hold() const { return !hold_used_this_piece_; }

  // Where the falling piece would land, for a ghost preview.
  Piece landing_position() const;

  void save(std::ostream& out) const;
  static std::optional<Game> load(std::istream& in);

  // Debug output, so the engine is playable before any terminal code exists.
  void print(std::ostream& out) const;

  friend bool operator==(const Game&, const Game&);

 private:
  // Leaves the board sized but every piece slot default filled; only the
  // deserialiser uses it, and it overwrites everything.
  Game(int rows, int cols);

  bool apply_move(Move move);
  void gravity_tick();
  void try_move(Offset delta);
  bool try_rotate(bool clockwise);
  void hard_drop();
  void hold();
  void lock_and_respawn();
  void clear_lines_and_score();

  Piece spawn(Tetromino type) const;
  Tetromino draw_from_bag();
  void reset_gravity();

  Board board_;
  Piece falling_;
  Piece next_;
  std::optional<Piece> held_;

  // Set by the hold path, cleared only by lock_and_respawn(). Clearing it in
  // a helper shared with the hold path would silently restore the original's
  // unlimited swapping, which stalls the game forever.
  bool hold_used_this_piece_ = false;

  int points_ = 0;
  int level_ = 0;
  int lines_remaining_ = kLinesPerLevel;
  int lines_cleared_ = 0;
  int ticks_till_gravity_ = 0;
  bool game_over_ = false;

  std::mt19937 rng_;
  std::vector<Tetromino> bag_;
};

}  // namespace tetris
