#include <catch2/catch_test_macros.hpp>

#include "support.hpp"
#include "tetris/game.hpp"
#include "tetris/tables.hpp"

using namespace tetris;
using namespace tetris::test;

TEST_CASE("a soft drop moves the piece down exactly one row") {
  Game game(20, 10, 1);
  const int before = top_row(game.falling());

  game.tick(Move::SoftDrop);

  REQUIRE(top_row(game.falling()) == before + 1);
}

TEST_CASE("a soft drop does not lock the piece") {
  Game game(20, 10, 1);
  const Tetromino type = game.falling().type();

  repeat(game, Move::SoftDrop, 3);

  REQUIRE(game.falling().type() == type);
  REQUIRE(occupied_count(game) == kCellsPerPiece);
  REQUIRE(game.can_hold());
}

// The whole point of the feature: the row taken by the soft drop is not also
// handed out by gravity in the same tick.
TEST_CASE("a soft drop restarts the gravity counter") {
  Game game(20, 10, 1);
  const int interval = kGravity[0];

  // One tick short of a gravity step, then soft drop. Gravity must not fire.
  repeat(game, Move::None, interval - 1);
  const int before = top_row(game.falling());
  game.tick(Move::SoftDrop);

  REQUIRE(top_row(game.falling()) == before + 1);

  // And the next tick must not fire it either, because the counter restarted.
  game.tick(Move::None);
  REQUIRE(top_row(game.falling()) == before + 1);
}

TEST_CASE("soft dropping reaches the landing row without locking early") {
  Game game(20, 10, 1);
  const Piece landing = game.landing_position();
  const int target = top_row(landing);
  const Tetromino type = game.falling().type();

  // Asserting inside the loop is the point: the piece must still be falling at
  // every row on the way down, not just once it arrives.
  while (top_row(game.falling()) < target) {
    game.tick(Move::SoftDrop);
    REQUIRE(game.falling().type() == type);
    REQUIRE(occupied_count(game) == kCellsPerPiece);
  }

  REQUIRE(top_row(game.falling()) == target);
}

TEST_CASE("a soft drop against the floor is a no-op, not a lock") {
  Game game(20, 10, 1);
  const Tetromino type = game.falling().type();
  const int target = top_row(game.landing_position());

  // Stop on arrival, so the gravity counter is exactly the interval that the
  // last successful row restarted.
  while (top_row(game.falling()) < target) game.tick(Move::SoftDrop);
  const int resting = top_row(game.falling());

  REQUIRE(resting == target);
  REQUIRE(game.falling().type() == type);

  // Still there one tick short of that interval. Two are subtracted, not one:
  // gravity also ran on the arrival tick, after the drop restarted it.
  repeat(game, Move::SoftDrop, kGravity[0] - 2);
  REQUIRE(top_row(game.falling()) == resting);
  REQUIRE(game.falling().type() == type);
}

TEST_CASE("a soft dropped piece still locks on the normal gravity schedule") {
  Game game(20, 10, 1);
  repeat(game, Move::SoftDrop, game.rows());
  REQUIRE(occupied_count(game) == kCellsPerPiece);

  // Resting on the floor, so the next gravity step has nowhere to go and locks.
  idle(game, kGravity[0]);

  REQUIRE(occupied_count(game) == 2 * kCellsPerPiece);
}

TEST_CASE("a soft drop onto a stack rests on top of it") {
  const std::uint32_t seed = seed_with_first_piece(Tetromino::O);
  Board board(20, 10);
  fill_row(board, 19);
  Game game(std::move(board), seed);

  const int prefilled = game.cols();
  repeat(game, Move::SoftDrop, game.rows());

  // Resting on the filled row, not merged into it and not through it.
  REQUIRE(bottom_row(game.falling()) == 18);
  REQUIRE(occupied_count(game) == prefilled + kCellsPerPiece);
}
