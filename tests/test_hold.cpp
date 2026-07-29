#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "support.hpp"
#include "tetris/game.hpp"
#include "tetris/tables.hpp"

using namespace tetris;
using namespace tetris::test;

TEST_CASE("a new game holds nothing and hold is available") {
  const Game game(20, 10, 7);
  REQUIRE_FALSE(game.held().has_value());
  REQUIRE(game.can_hold());
}

TEST_CASE("holding on an empty slot stores the piece and pulls from the bag") {
  Game game(20, 10, 7);
  const Tetromino first = game.falling().type();
  const Tetromino second = game.next().type();

  game.tick(Move::Hold);

  REQUIRE(game.held().has_value());
  REQUIRE(game.held()->type() == first);
  REQUIRE(game.falling().type() == second);
  REQUIRE(game.next().type() != second);  // a fresh piece was drawn
  REQUIRE_FALSE(game.can_hold());
}

TEST_CASE("holding on a full slot swaps without touching the bag") {
  Game game(20, 10, 7);
  const Tetromino stored = game.falling().type();
  game.tick(Move::Hold);

  game.tick(Move::Drop);  // lock, which clears the lockout
  REQUIRE(game.can_hold());

  const Tetromino current = game.falling().type();
  const Tetromino queued = game.next().type();

  game.tick(Move::Hold);

  REQUIRE(game.falling().type() == stored);   // the stored piece comes back
  REQUIRE(game.held()->type() == current);    // and the current one goes in
  REQUIRE(game.next().type() == queued);      // the bag was not touched
}

TEST_CASE("a second hold on the same piece is rejected") {
  Game game(20, 10, 7);
  game.tick(Move::Hold);
  REQUIRE_FALSE(game.can_hold());

  const Game after_first_hold = game;
  game.tick(Move::Hold);

  // The rejected hold still costs a tick of gravity, so compare everything
  // that the hold itself would have changed.
  REQUIRE(game.falling().type() == after_first_hold.falling().type());
  REQUIRE(game.next().type() == after_first_hold.next().type());
  REQUIRE(game.held()->type() == after_first_hold.held()->type());
  REQUIRE_FALSE(game.can_hold());
}

TEST_CASE("the lockout clears when the piece locks, not when it is held") {
  Game game(20, 10, 7);

  game.tick(Move::Hold);
  REQUIRE_FALSE(game.can_hold());

  // Plenty of ticks, but no lock: the piece is still falling.
  idle(game, 10);
  REQUIRE_FALSE(game.can_hold());

  game.tick(Move::Drop);
  REQUIRE(game.can_hold());
}

TEST_CASE("the lockout clears after a gravity lock too") {
  Game game(20, 10, 7);
  game.tick(Move::Hold);
  REQUIRE_FALSE(game.can_hold());

  // Fall all the way to the floor and sit there until the piece locks.
  const int distance = game.landing_position().origin().row -
                       game.falling().origin().row;
  idle(game, kGravity[0] * (distance + 1) + 1);

  REQUIRE(game.can_hold());
}

TEST_CASE("a held piece returns upright at the spawn position") {
  Game game(20, 10, 7);
  game.tick(Move::RotateCW);
  game.tick(Move::RotateCW);
  repeat(game, Move::Left, 3);
  REQUIRE(game.falling().orientation() == 2);

  const Tetromino stashed = game.falling().type();
  game.tick(Move::Hold);

  REQUIRE(game.held()->type() == stashed);
  REQUIRE(game.held()->orientation() == 0);
  REQUIRE(game.held()->origin() == Offset{0, game.cols() / 2 - 2});
}

TEST_CASE("hold never stalls the game the way unlimited swapping would") {
  // The original respawned the piece on every hold, so leaning on the key
  // kept the board empty forever. Once per piece, the stack still grows.
  Game game(20, 10, 7);
  for (int i = 0; i < 6000 && !game.game_over(); ++i) game.tick(Move::Hold);

  int settled = 0;
  for (int row = 0; row < game.rows(); ++row) {
    for (int col = 0; col < game.cols(); ++col) {
      if (!is_empty(game.at(row, col))) ++settled;
    }
  }
  REQUIRE(settled > 2 * kCellsPerPiece);  // pieces have been landing all along
}

namespace {

// The order the bag deals types in: the two pieces a new game starts with,
// then one fresh draw per lock. The board is tall enough that stacking in the
// spawn columns never reaches the top and never clears a line, so the run is
// exactly one draw per drop.
std::vector<Tetromino> draw_sequence(int draws, std::uint32_t seed) {
  Game game(4 * draws + 40, 10, seed);
  std::vector<Tetromino> sequence{game.falling().type(), game.next().type()};
  while (static_cast<int>(sequence.size()) < draws) {
    game.tick(Move::Drop);
    REQUIRE_FALSE(game.game_over());
    sequence.push_back(game.next().type());
  }
  return sequence;
}

}  // namespace

TEST_CASE("a bag deals all seven types before repeating any") {
  const auto sequence = draw_sequence(70, 4242);

  for (std::size_t start = 0; start + kTetrominoCount <= sequence.size();
       start += kTetrominoCount) {
    const auto first = sequence.begin() + static_cast<std::ptrdiff_t>(start);
    const std::set<Tetromino> bag(first, first + kTetrominoCount);
    INFO("bag starting at " << start);
    REQUIRE(bag.size() == kTetrominoCount);
  }
}

TEST_CASE("every type appears equally often over many bags") {
  const auto sequence = draw_sequence(7 * 40, 31337);

  std::map<Tetromino, int> counts;
  for (Tetromino type : sequence) ++counts[type];

  REQUIRE(static_cast<int>(counts.size()) == kTetrominoCount);
  for (const auto& [type, count] : counts) {
    INFO("type " << static_cast<int>(type));
    REQUIRE(count == 40);
  }
}

TEST_CASE("different seeds deal different orders") {
  REQUIRE(draw_sequence(70, 1) != draw_sequence(70, 2));
  REQUIRE(draw_sequence(70, 1) == draw_sequence(70, 1));
}
