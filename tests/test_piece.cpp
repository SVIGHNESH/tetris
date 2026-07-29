#include <catch2/catch_test_macros.hpp>

#include <set>

#include "tetris/piece.hpp"

using namespace tetris;

namespace {

std::set<std::pair<int, int>> cell_set(const Piece& piece) {
  std::set<std::pair<int, int>> out;
  for (const Offset& c : piece.cells()) out.emplace(c.row, c.col);
  return out;
}

}  // namespace

TEST_CASE("cells() are absolute board coordinates") {
  const Piece at_origin(Tetromino::I, {0, 0});
  const Piece moved = at_origin.translated({3, 5});

  auto expected = cell_set(at_origin);
  std::set<std::pair<int, int>> shifted;
  for (const auto& [row, col] : expected) shifted.emplace(row + 3, col + 5);

  REQUIRE(cell_set(moved) == shifted);
  REQUIRE(moved.origin() == Offset{3, 5});
}

TEST_CASE("four rotations return to the starting orientation") {
  for (int type = 0; type < kTetrominoCount; ++type) {
    const Piece start(static_cast<Tetromino>(type), {2, 2});
    SECTION("clockwise") {
      Piece p = start;
      for (int i = 0; i < kOrientations; ++i) p = p.rotated(true);
      REQUIRE(p == start);
    }
    SECTION("counter-clockwise") {
      Piece p = start;
      for (int i = 0; i < kOrientations; ++i) p = p.rotated(false);
      REQUIRE(p == start);
    }
  }
}

TEST_CASE("clockwise and counter-clockwise are inverses") {
  for (int type = 0; type < kTetrominoCount; ++type) {
    const Piece start(static_cast<Tetromino>(type), {1, 1});
    REQUIRE(start.rotated(true).rotated(false) == start);
    REQUIRE(start.rotated(false).rotated(true) == start);
  }
}

TEST_CASE("the O piece is unchanged by rotation") {
  const Piece o(Tetromino::O, {4, 4});
  for (int i = 1; i < kOrientations; ++i) {
    Piece rotated = o;
    for (int j = 0; j < i; ++j) rotated = rotated.rotated(true);
    REQUIRE(cell_set(rotated) == cell_set(o));
  }
}

TEST_CASE("rotation does not move the origin") {
  const Piece t(Tetromino::T, {7, 3});
  REQUIRE(t.rotated(true).origin() == t.origin());
  REQUIRE(t.rotated(false).origin() == t.origin());
}

TEST_CASE("rotating and translating leave the source piece untouched") {
  const Piece original(Tetromino::L, {5, 5});
  const auto before = cell_set(original);
  (void)original.rotated(true);
  (void)original.translated({9, 9});
  REQUIRE(cell_set(original) == before);
}

TEST_CASE("cell() maps the type onto the matching board cell") {
  REQUIRE(Piece(Tetromino::I, {0, 0}).cell() == Cell::I);
  REQUIRE(Piece(Tetromino::Z, {0, 0}).cell() == Cell::Z);
  REQUIRE(Piece(Tetromino::O, {0, 0}).cell() != Cell::Empty);
}
