#include <catch2/catch_test_macros.hpp>

#include "support.hpp"
#include "tetris/game.hpp"
#include "tetris/tables.hpp"

using namespace tetris;
using namespace tetris::test;

namespace {

// Fills every row from `from` down so that only the columns the first piece
// will land in are left open, then drops it. Returns the game afterwards.
Game drop_into_primed_board(std::uint32_t seed, int& rows_completed,
                            int& rows_fallen) {
  auto primed = prime_for_first_drop(20, 10, seed);
  rows_completed = primed.rows_completed;
  Game game(std::move(primed.board), seed);
  rows_fallen =
      game.landing_position().origin().row - game.falling().origin().row;
  game.tick(Move::Drop);
  return game;
}

}  // namespace

TEST_CASE("clearing lines scores from the points table") {
  // A horizontal I completes exactly one row; a vertical I would complete
  // four, but the piece arrives unrotated.
  const std::uint32_t seed = seed_with_first_piece(Tetromino::I);
  int completed = 0;
  int fell = 0;
  const Game game = drop_into_primed_board(seed, completed, fell);

  REQUIRE(completed == 1);
  REQUIRE(game.lines_cleared() == 1);
  // Level 0, multiplier 1, plus the hard drop's own points.
  REQUIRE(game.score() == kLineMultiplier[1] + kHardDropPoints * fell);
}

TEST_CASE("a two-row piece scores a double") {
  const std::uint32_t seed = seed_with_first_piece(Tetromino::O);
  int completed = 0;
  int fell = 0;
  const Game game = drop_into_primed_board(seed, completed, fell);

  REQUIRE(completed == 2);
  REQUIRE(game.lines_cleared() == 2);
  REQUIRE(game.score() == kLineMultiplier[2] + kHardDropPoints * fell);
}

TEST_CASE("a vertical I clears four rows for a tetris") {
  const std::uint32_t seed = seed_with_first_piece(Tetromino::I);

  // Find where a vertical I lands on an empty board, then leave exactly that
  // footprint open.
  Game probe(20, 10, seed);
  probe.tick(Move::RotateCW);
  const auto footprint = probe.landing_position().cells();

  Board board(20, 10);
  for (const Offset& c : footprint) fill_row_except(board, c.row, {c.col});

  Game game(std::move(board), seed);
  game.tick(Move::RotateCW);
  const int fell =
      game.landing_position().origin().row - game.falling().origin().row;
  game.tick(Move::Drop);

  REQUIRE(game.lines_cleared() == 4);
  REQUIRE(game.score() == kLineMultiplier[4] + kHardDropPoints * fell);
  REQUIRE(occupied_count(game) == kCellsPerPiece);  // only the new piece
}

TEST_CASE("the level tracks the line count through a long played game") {
  Game game(20, 10, 2024);

  int highest_level = 0;
  int previous_score = 0;
  int previous_lines = 0;
  int previous_level = 0;

  const int drops = play_greedy(game, 2000, [&](const Game& state, int fell) {
    // The engine's level bookkeeping carries overshoot forward, so this holds
    // no matter how many lines a single drop clears.
    REQUIRE(state.level() ==
            std::min(kMaxLevel, state.lines_cleared() / kLinesPerLevel));
    REQUIRE(state.lines_remaining() >= 1);
    REQUIRE(state.lines_remaining() <= kLinesPerLevel);

    // Every clear along the way is checked against the points table at
    // whatever level it happened on, which covers the multiplier across the
    // whole range rather than only at level 0.
    const int cleared = state.lines_cleared() - previous_lines;
    REQUIRE(cleared >= 0);
    REQUIRE(cleared <= kCellsPerPiece);
    REQUIRE(state.score() - previous_score ==
            kLineMultiplier[static_cast<std::size_t>(cleared)] *
                    (previous_level + 1) +
                kHardDropPoints * fell);

    previous_score = state.score();
    previous_lines = state.lines_cleared();
    previous_level = state.level();
    highest_level = std::max(highest_level, state.level());
  });

  INFO("survived " << drops << " pieces, cleared " << game.lines_cleared());
  REQUIRE(game.lines_cleared() >= kLinesPerLevel * (kMaxLevel + 1));
  REQUIRE(highest_level == kMaxLevel);
  REQUIRE(game.score() > 0);
}

TEST_CASE("gravity gets faster as the level rises") {
  // A freshly spawned piece takes kGravity[level] ticks to fall one row, so
  // timing that fall reads back the interval the engine is using. Every hard
  // drop respawns, so the falling piece is always fresh here.
  auto ticks_to_fall_one_row = [](Game& game) {
    const int start = game.falling().origin().row;
    int ticks = 0;
    while (game.falling().origin().row == start && ticks < 1000) {
      game.tick(Move::None);
      ++ticks;
    }
    return ticks;
  };

  Game game(20, 10, 2024);
  REQUIRE(game.level() == 0);
  REQUIRE(ticks_to_fall_one_row(game) == kGravity[0]);

  while (game.level() == 0 && !game.game_over()) {
    apply(game, best_placement(game));
  }
  REQUIRE(game.level() > 0);
  REQUIRE(ticks_to_fall_one_row(game) == kGravity[game.level()]);
  REQUIRE(kGravity[game.level()] < kGravity[0]);
}

TEST_CASE("a drop that clears nothing scores only the drop points") {
  Game game(20, 10, 7);
  const int fell =
      game.landing_position().origin().row - game.falling().origin().row;
  game.tick(Move::Drop);
  REQUIRE(game.score() == kHardDropPoints * fell);
  REQUIRE(game.lines_cleared() == 0);
  REQUIRE(game.lines_remaining() == kLinesPerLevel);
}

TEST_CASE("a soft drop scores one point per row taken") {
  Game game(20, 10, 7);
  repeat(game, Move::SoftDrop, 3);
  REQUIRE(game.score() == 3 * kSoftDropPoints);

  // Against the floor the drop fails, so no further points accrue.
  repeat(game, Move::SoftDrop, game.rows());
  const int rows_taken = game.score() / kSoftDropPoints;
  repeat(game, Move::SoftDrop, 5);
  REQUIRE(game.score() == rows_taken * kSoftDropPoints);
}
