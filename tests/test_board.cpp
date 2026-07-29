#include <catch2/catch_test_macros.hpp>

#include "tetris/board.hpp"

using namespace tetris;

namespace {

void fill_row(Board& board, int row, Cell cell = Cell::I) {
  for (int col = 0; col < board.cols(); ++col) board.set(row, col, cell);
}

// A full row minus one hole, which is what a row looks like right before a
// clear.
void fill_row_except(Board& board, int row, int hole, Cell cell = Cell::I) {
  fill_row(board, row, cell);
  board.set(row, hole, Cell::Empty);
}

int occupied_count(const Board& board) {
  int count = 0;
  for (int row = 0; row < board.rows(); ++row) {
    for (int col = 0; col < board.cols(); ++col) {
      if (!is_empty(board.at(row, col))) ++count;
    }
  }
  return count;
}

}  // namespace

TEST_CASE("a new board is empty and knows its dimensions") {
  const Board board(20, 10);
  REQUIRE(board.rows() == 20);
  REQUIRE(board.cols() == 10);
  REQUIRE(occupied_count(board) == 0);
}

TEST_CASE("in_bounds rejects every direction") {
  const Board board(20, 10);
  REQUIRE(board.in_bounds(0, 0));
  REQUIRE(board.in_bounds(19, 9));
  REQUIRE_FALSE(board.in_bounds(-1, 0));
  REQUIRE_FALSE(board.in_bounds(0, -1));
  REQUIRE_FALSE(board.in_bounds(20, 0));
  REQUIRE_FALSE(board.in_bounds(0, 10));
}

TEST_CASE("fits is a pure query") {
  Board board(20, 10);
  const Piece piece(Tetromino::O, {0, 4});

  REQUIRE(board.fits(piece));
  REQUIRE(occupied_count(board) == 0);  // no side effects

  board.lock(piece);
  REQUIRE_FALSE(board.fits(piece));
  REQUIRE(occupied_count(board) == 4);
}

TEST_CASE("fits rejects a piece hanging off the edges") {
  const Board board(20, 10);
  const Piece piece(Tetromino::O, {0, 0});
  REQUIRE_FALSE(board.fits(piece.translated({0, -2})));
  REQUIRE_FALSE(board.fits(piece.translated({0, 9})));
  REQUIRE_FALSE(board.fits(piece.translated({19, 0})));
}

TEST_CASE("lock stamps the piece type into the board") {
  Board board(20, 10);
  const Piece piece(Tetromino::T, {5, 3});
  board.lock(piece);
  for (const Offset& c : piece.cells()) {
    REQUIRE(board.at(c.row, c.col) == Cell::T);
  }
}

TEST_CASE("a single line clear drops everything above it") {
  Board board(20, 10);
  board.set(17, 2, Cell::T);  // a lone cell that should fall one row
  fill_row(board, 18);
  board.set(19, 5, Cell::S);  // untouched row below the clear

  REQUIRE(board.clear_full_lines() == 1);

  REQUIRE(board.at(18, 2) == Cell::T);
  REQUIRE(is_empty(board.at(17, 2)));
  REQUIRE(board.at(19, 5) == Cell::S);
  REQUIRE(occupied_count(board) == 2);
}

TEST_CASE("double, triple and tetris clears") {
  auto clear_n = [](int n) {
    Board board(20, 10);
    for (int i = 0; i < n; ++i) fill_row(board, 19 - i);
    return board.clear_full_lines();
  };
  REQUIRE(clear_n(2) == 2);
  REQUIRE(clear_n(3) == 3);
  REQUIRE(clear_n(4) == 4);
}

TEST_CASE("non-adjacent cleared rows collapse correctly") {
  Board board(20, 10);
  board.set(14, 0, Cell::T);  // marker A, above both cleared rows
  fill_row(board, 15);
  board.set(16, 1, Cell::S);  // marker B, between the cleared rows
  fill_row(board, 17);
  board.set(18, 2, Cell::Z);  // marker C, below both cleared rows

  REQUIRE(board.clear_full_lines() == 2);

  // A falls two rows, B falls one, C stays put.
  REQUIRE(board.at(16, 0) == Cell::T);
  REQUIRE(board.at(17, 1) == Cell::S);
  REQUIRE(board.at(18, 2) == Cell::Z);
  REQUIRE(occupied_count(board) == 3);
}

TEST_CASE("clearing every row empties the board") {
  Board board(20, 10);
  for (int row = 0; row < 20; ++row) fill_row(board, row);
  REQUIRE(board.clear_full_lines() == 20);
  REQUIRE(occupied_count(board) == 0);
}

TEST_CASE("a row with a single hole is not cleared") {
  Board board(20, 10);
  fill_row_except(board, 19, 4);
  REQUIRE(board.clear_full_lines() == 0);
  REQUIRE(occupied_count(board) == 9);
}

TEST_CASE("boards of unusual sizes behave") {
  Board board(4, 3);
  fill_row(board, 3);
  REQUIRE(board.clear_full_lines() == 1);
  REQUIRE(occupied_count(board) == 0);
}
