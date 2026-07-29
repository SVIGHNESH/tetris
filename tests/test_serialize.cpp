#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "support.hpp"
#include "tetris/game.hpp"

using namespace tetris;
using namespace tetris::test;

namespace {

Game round_trip(const Game& game) {
  std::stringstream stream;
  game.save(stream);
  auto loaded = Game::load(stream);
  REQUIRE(loaded.has_value());
  return *loaded;
}

std::vector<Tetromino> upcoming(Game game, int count) {
  std::vector<Tetromino> types;
  for (int i = 0; i < count; ++i) {
    types.push_back(game.next().type());
    game.tick(Move::Drop);
  }
  return types;
}

}  // namespace

TEST_CASE("a fresh game survives a save and load unchanged") {
  const Game game(20, 10, 12345);
  REQUIRE(round_trip(game) == game);
}

TEST_CASE("a game in progress survives a save and load unchanged") {
  Game game(20, 10, 99);
  play_greedy(game, 40, [](const Game&) {});
  game.tick(Move::Hold);
  idle(game, 17);
  game.tick(Move::RotateCW);
  game.tick(Move::Left);

  const Game loaded = round_trip(game);
  REQUIRE(loaded == game);
  REQUIRE(loaded.score() == game.score());
  REQUIRE(loaded.level() == game.level());
  REQUIRE(loaded.lines_cleared() == game.lines_cleared());
  REQUIRE(loaded.can_hold() == game.can_hold());
  REQUIRE(loaded.held().has_value());
  REQUIRE(loaded.held()->type() == game.held()->type());
  REQUIRE(loaded.falling() == game.falling());
  REQUIRE(loaded.next() == game.next());

  for (int row = 0; row < game.rows(); ++row) {
    for (int col = 0; col < game.cols(); ++col) {
      REQUIRE(loaded.at(row, col) == game.at(row, col));
    }
  }
}

TEST_CASE("the upcoming piece sequence is identical after a load") {
  Game game(120, 10, 555);
  play_greedy(game, 12, [](const Game&) {});

  const Game loaded = round_trip(game);
  REQUIRE(upcoming(loaded, 30) == upcoming(game, 30));
}

TEST_CASE("the hold lockout state travels with the save") {
  Game held_used(20, 10, 7);
  held_used.tick(Move::Hold);
  REQUIRE_FALSE(held_used.can_hold());
  REQUIRE_FALSE(round_trip(held_used).can_hold());

  const Game fresh(20, 10, 7);
  REQUIRE(fresh.can_hold());
  REQUIRE(round_trip(fresh).can_hold());
}

TEST_CASE("boards of unusual sizes round trip") {
  Game game(31, 17, 8);
  idle(game, 300);
  const Game loaded = round_trip(game);
  REQUIRE(loaded.rows() == 31);
  REQUIRE(loaded.cols() == 17);
  REQUIRE(loaded == game);
}

TEST_CASE("a finished game round trips as finished") {
  Board board(20, 10);
  for (int row = 1; row < 20; ++row) fill_row_except(board, row, {0});
  const Game game(std::move(board), 7);
  REQUIRE(game.game_over());
  REQUIRE(round_trip(game).game_over());
}

TEST_CASE("loading rejects anything that is not a save file") {
  auto rejects = [](const std::string& text) {
    std::stringstream stream(text);
    return !Game::load(stream).has_value();
  };

  REQUIRE(rejects(""));
  REQUIRE(rejects("hello"));
  REQUIRE(rejects("TETRIS-SAVE"));            // magic but nothing after it
  REQUIRE(rejects("TETRIS-SAVE\n2\n20 10\n"));  // a future format version
  REQUIRE(rejects("TETRIS-SAVE\n1\n0 10\n"));   // a board with no rows
}

TEST_CASE("loading rejects a truncated save") {
  std::stringstream full;
  Game(20, 10, 7).save(full);
  const std::string text = full.str();

  for (std::size_t cut : {text.size() / 4, text.size() / 2,
                          text.size() * 3 / 4, text.size() - 2}) {
    std::stringstream truncated(text.substr(0, cut));
    INFO("truncated to " << cut << " of " << text.size() << " bytes");
    REQUIRE_FALSE(Game::load(truncated).has_value());
  }
}

TEST_CASE("loading rejects out of range field values") {
  std::stringstream full;
  Game(20, 10, 7).save(full);
  std::string text = full.str();

  // The level sits on the fourth line; push it past the maximum.
  const std::size_t level_line = text.find('\n', text.find('\n', 12) + 1) + 1;
  std::string corrupted = text;
  corrupted.replace(level_line, corrupted.find('\n', level_line) - level_line,
                    "0 99 10 0 50 0 0");

  std::stringstream stream(corrupted);
  REQUIRE_FALSE(Game::load(stream).has_value());
}

TEST_CASE("a loaded game keeps playing from where it left off") {
  Game game(20, 10, 4321);
  play_greedy(game, 25, [](const Game&) {});

  Game loaded = round_trip(game);
  for (int i = 0; i < 300; ++i) {
    game.tick(Move::None);
    loaded.tick(Move::None);
  }
  REQUIRE(loaded == game);
}
