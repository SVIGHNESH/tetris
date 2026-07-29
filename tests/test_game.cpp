#include <catch2/catch_test_macros.hpp>

#include "support.hpp"
#include "tetris/game.hpp"
#include "tetris/tables.hpp"

using namespace tetris;
using namespace tetris::test;

TEST_CASE("a new game spawns a piece at the top, centred") {
  const Game game(20, 10, 1234);
  REQUIRE(game.falling().origin() == Offset{0, 3});
  REQUIRE(game.falling().orientation() == 0);
  REQUIRE_FALSE(game.game_over());
  REQUIRE(game.score() == 0);
  REQUIRE(game.level() == 0);
  REQUIRE(game.lines_remaining() == kLinesPerLevel);
}

TEST_CASE("the same seed replays the same game") {
  Game a(20, 10, 99);
  Game b(20, 10, 99);
  for (int i = 0; i < 500; ++i) {
    a.tick(Move::None);
    b.tick(Move::None);
  }
  REQUIRE(a == b);
}

TEST_CASE("gravity drops the piece one row per interval") {
  Game game(20, 10, 7);
  const int interval = kGravity[0];

  idle(game, interval - 1);
  REQUIRE(game.falling().origin().row == 0);

  idle(game, 1);
  REQUIRE(game.falling().origin().row == 1);

  idle(game, interval);
  REQUIRE(game.falling().origin().row == 2);
}

TEST_CASE("left and right move the piece and stop at the walls") {
  Game game(20, 10, 7);
  const Offset start = game.falling().origin();

  game.tick(Move::Left);
  REQUIRE(game.falling().origin().col == start.col - 1);

  game.tick(Move::Right);
  REQUIRE(game.falling().origin().col == start.col);

  repeat(game, Move::Left, 20);
  const int leftmost = game.falling().origin().col;
  game.tick(Move::Left);
  REQUIRE(game.falling().origin().col == leftmost);
  for (const Offset& c : game.falling().cells()) REQUIRE(c.col >= 0);

  repeat(game, Move::Right, 40);
  const int rightmost = game.falling().origin().col;
  game.tick(Move::Right);
  REQUIRE(game.falling().origin().col == rightmost);
  for (const Offset& c : game.falling().cells()) REQUIRE(c.col < game.cols());
}

TEST_CASE("a hard drop lands the piece and spawns the next one") {
  Game game(20, 10, 7);
  const Piece expected_next = game.next();
  const Piece landing = game.landing_position();

  game.tick(Move::Drop);

  REQUIRE(game.falling().type() == expected_next.type());
  REQUIRE(game.falling().origin() == expected_next.origin());
  // The dropped piece is now settled where it landed.
  for (const Offset& c : landing.cells()) {
    REQUIRE(game.at(c.row, c.col) == landing.cell());
  }
}

TEST_CASE("a piece resting on the floor locks and gravity restarts") {
  Game game(20, 10, 7);
  const Piece landing = game.landing_position();

  // Fall to the floor, then sit there for one more gravity interval.
  const int distance = landing.origin().row - game.falling().origin().row;
  idle(game, kGravity[0] * distance);
  REQUIRE(game.falling().origin() == landing.origin());

  idle(game, kGravity[0]);
  REQUIRE(game.falling().origin().row == 0);  // a fresh piece at the top
}

TEST_CASE("game over when the spawning piece has nowhere to go") {
  // Every row is full except column 0, so nothing clears and the spawn area
  // is already occupied.
  Board board(20, 10);
  for (int row = 1; row < 20; ++row) fill_row_except(board, row, {0});

  const Game game(std::move(board), 7);
  REQUIRE(game.game_over());
}

TEST_CASE("game over eventually fires as the stack reaches the top") {
  Board board(20, 10);
  for (int row = 4; row < 20; ++row) fill_row_except(board, row, {9});
  Game game(std::move(board), 7);
  REQUIRE_FALSE(game.game_over());

  bool over = false;
  for (int i = 0; i < 20 && !over; ++i) {
    over = game.tick(Move::Drop) == TickResult::GameOver;
  }
  REQUIRE(over);
  REQUIRE(game.game_over());
}

TEST_CASE("ticking a finished game is a no-op") {
  Board board(20, 10);
  for (int row = 1; row < 20; ++row) fill_row_except(board, row, {0});
  Game game(std::move(board), 7);
  REQUIRE(game.game_over());

  const Game before = game;
  REQUIRE(game.tick(Move::Drop) == TickResult::GameOver);
  REQUIRE(game == before);
}

TEST_CASE("at() composites the falling piece over the settled board") {
  Game game(20, 10, 7);
  const auto cells = game.falling().cells();
  for (const Offset& c : cells) {
    REQUIRE(game.at(c.row, c.col) == game.falling().cell());
  }
  REQUIRE(occupied_count(game) == kCellsPerPiece);
}

TEST_CASE("rotation turns in place when there is room") {
  Game game(20, 10, seed_with_first_piece(Tetromino::T));
  const Offset origin = game.falling().origin();

  game.tick(Move::RotateCW);
  REQUIRE(game.falling().orientation() == 1);
  REQUIRE(game.falling().origin() == origin);

  game.tick(Move::RotateCCW);
  REQUIRE(game.falling().orientation() == 0);
  REQUIRE(game.falling().origin() == origin);
}

TEST_CASE("an I piece kicks off the right wall") {
  Game game(20, 10, seed_with_first_piece(Tetromino::I));

  game.tick(Move::RotateCW);  // vertical
  repeat(game, Move::Right, 20);
  const Offset against_wall = game.falling().origin();

  game.tick(Move::RotateCW);  // horizontal again, needs room to the left
  REQUIRE(game.falling().orientation() == 2);
  REQUIRE(game.falling().origin().col < against_wall.col);
  for (const Offset& c : game.falling().cells()) {
    REQUIRE(c.col < game.cols());
    REQUIRE(c.col >= 0);
  }
}

TEST_CASE("an I piece kicks off the left wall") {
  Game game(20, 10, seed_with_first_piece(Tetromino::I));

  game.tick(Move::RotateCW);  // vertical
  repeat(game, Move::Left, 20);
  const Offset against_wall = game.falling().origin();

  game.tick(Move::RotateCW);  // horizontal again, needs room to the right
  REQUIRE(game.falling().orientation() == 2);
  REQUIRE(game.falling().origin().col > against_wall.col);
  for (const Offset& c : game.falling().cells()) {
    REQUIRE(c.col >= 0);
    REQUIRE(c.col < game.cols());
  }
}

TEST_CASE("rotation fails when a one-wide well leaves genuinely no room") {
  // Columns 0 and 2 are solid from row 4 down, leaving column 1 as a well
  // that is open at the top.
  Board board(20, 10);
  for (int row = 4; row < 20; ++row) {
    board.set(row, 0, Cell::T);
    board.set(row, 2, Cell::T);
  }

  Game game(std::move(board), seed_with_first_piece(Tetromino::I));

  game.tick(Move::RotateCW);   // vertical, in column 5
  repeat(game, Move::Left, 4);  // into the well at column 1
  REQUIRE(game.falling().origin().col == -1);

  idle(game, kGravity[0]);  // descend so the walls flank the piece
  REQUIRE(game.falling().origin().row == 1);

  const Piece before = game.falling();
  game.tick(Move::RotateCW);
  REQUIRE(game.falling().orientation() == before.orientation());
  REQUIRE(game.falling().origin() == before.origin());
}

TEST_CASE("landing_position does not disturb the game") {
  Game game(20, 10, 7);
  const Game before = game;
  (void)game.landing_position();
  REQUIRE(game == before);
}

TEST_CASE("boards of other sizes still play") {
  Game game(30, 16, 11);
  REQUIRE(game.rows() == 30);
  REQUIRE(game.cols() == 16);
  REQUIRE(game.falling().origin().col == 6);
  REQUIRE(idle(game, 200) == TickResult::Continue);
}
