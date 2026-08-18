#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "support.hpp"
#include "tetris/game.hpp"
#include "tetris/tables.hpp"

using namespace tetris;
using namespace tetris::test;

namespace {

// Soft drops the falling piece to its landing row. The arrival tick counts as
// the first tick of the lock delay, which every count below accounts for.
void ground(Game& game) {
  const int target = top_row(game.landing_position());
  while (top_row(game.falling()) < target) game.tick(Move::SoftDrop);
}

}  // namespace

TEST_CASE("a grounded piece survives the whole lock delay before locking") {
  Game game(20, 10, 1);
  ground(game);

  idle(game, kLockDelayTicks - 2);
  REQUIRE(occupied_count(game) == kCellsPerPiece);

  idle(game, 1);
  REQUIRE(occupied_count(game) == 2 * kCellsPerPiece);
}

TEST_CASE("moving a grounded piece restarts the lock delay") {
  Game game(20, 10, 1);
  ground(game);

  // Long enough that the piece would have locked without the restart.
  idle(game, kLockDelayTicks - 2);
  game.tick(Move::Left);
  idle(game, kLockDelayTicks - 2);

  REQUIRE(occupied_count(game) == kCellsPerPiece);
}

TEST_CASE("rotating a grounded piece restarts the lock delay") {
  // An O rotates in place, so the rotation succeeds even flush with the
  // floor; other types can fail to fit there, which would reset nothing.
  Game game(20, 10, seed_with_first_piece(Tetromino::O));
  ground(game);

  idle(game, kLockDelayTicks - 2);
  game.tick(Move::RotateCW);
  idle(game, kLockDelayTicks - 2);

  REQUIRE(occupied_count(game) == kCellsPerPiece);
}

TEST_CASE("the reset cap keeps a piece from being wiggled forever") {
  Game game(20, 10, 1);
  ground(game);

  // Alternate left and right every tick. Each successful move would restart
  // the delay, so without the cap this loop never locks.
  const int budget = kLockDelayTicks * (kMaxLockResets + 2);
  bool locked = false;
  for (int i = 0; i < budget && !locked; ++i) {
    game.tick(i % 2 == 0 ? Move::Left : Move::Right);
    locked = occupied_count(game) > kCellsPerPiece;
  }

  REQUIRE(locked);
}

TEST_CASE("a hard drop ignores the lock delay and locks at once") {
  Game game(20, 10, 1);
  game.tick(Move::Drop);
  REQUIRE(occupied_count(game) == kCellsPerPiece + kCellsPerPiece);
}

TEST_CASE("an airborne move does not consume a lock delay reset") {
  Game game(20, 10, 1);

  // Burn every reset airborne. If these counted, the grounded move below
  // could not restart the delay and the piece would lock early.
  for (int i = 0; i < kMaxLockResets + 1; ++i) {
    game.tick(i % 2 == 0 ? Move::Left : Move::Right);
  }

  ground(game);
  idle(game, kLockDelayTicks - 2);
  game.tick(Move::Left);
  idle(game, kLockDelayTicks - 2);

  REQUIRE(occupied_count(game) == kCellsPerPiece);
}

TEST_CASE("a mid-delay game survives a save and load round trip") {
  Game game(20, 10, 1);
  ground(game);
  idle(game, kLockDelayTicks / 2);

  std::ostringstream out;
  game.save(out);
  std::istringstream in(out.str());
  const auto loaded = Game::load(in);

  REQUIRE(loaded.has_value());
  REQUIRE(*loaded == game);
}
