#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <vector>

#include "tetris/tables.hpp"

using namespace tetris;

namespace {

// The bounding box of a shape, normalised so the top-left occupied cell sits
// at (0, 0). Two orientations describe the same tetromino only if their
// normalised cell sets are rotations of one another.
std::set<std::pair<int, int>> normalised(std::span<const Offset> cells) {
  int min_row = cells[0].row;
  int min_col = cells[0].col;
  for (const Offset& c : cells) {
    min_row = std::min(min_row, c.row);
    min_col = std::min(min_col, c.col);
  }
  std::set<std::pair<int, int>> out;
  for (const Offset& c : cells) out.emplace(c.row - min_row, c.col - min_col);
  return out;
}

std::set<std::pair<int, int>> rotate_cw(
    const std::set<std::pair<int, int>>& cells) {
  int max_row = 0;
  for (const auto& [row, col] : cells) max_row = std::max(max_row, row);
  std::vector<Offset> rotated;
  for (const auto& [row, col] : cells) rotated.push_back({col, max_row - row});
  return normalised(rotated);
}

}  // namespace

TEST_CASE("every orientation has exactly four distinct cells") {
  for (int type = 0; type < kTetrominoCount; ++type) {
    for (int ori = 0; ori < kOrientations; ++ori) {
      const auto& cells = kTetrominos[static_cast<std::size_t>(type)]
                                     [static_cast<std::size_t>(ori)];
      std::set<std::pair<int, int>> distinct;
      for (const Offset& c : cells) distinct.emplace(c.row, c.col);
      INFO("type " << type << " orientation " << ori);
      REQUIRE(distinct.size() == 4);
    }
  }
}

TEST_CASE("every orientation fits inside the 4x4 origin box") {
  for (const auto& type : kTetrominos) {
    for (const auto& ori : type) {
      for (const Offset& c : ori) {
        REQUIRE(c.row >= 0);
        REQUIRE(c.col >= 0);
        REQUIRE(c.row < 4);
        REQUIRE(c.col < 4);
      }
    }
  }
}

TEST_CASE("each orientation is the clockwise rotation of the previous one") {
  for (int type = 0; type < kTetrominoCount; ++type) {
    const auto& shape = kTetrominos[static_cast<std::size_t>(type)];
    for (int ori = 0; ori < kOrientations; ++ori) {
      const auto next = static_cast<std::size_t>((ori + 1) % kOrientations);
      INFO("type " << type << " orientation " << ori << " -> " << next);
      REQUIRE(rotate_cw(normalised(shape[static_cast<std::size_t>(ori)])) ==
              normalised(shape[next]));
    }
  }
}

TEST_CASE("gravity shortens monotonically as the level rises") {
  for (std::size_t level = 1; level < kGravity.size(); ++level) {
    REQUIRE(kGravity[level] < kGravity[level - 1]);
  }
  REQUIRE(kGravity[0] > 0);
  REQUIRE(kGravity.back() > 0);
}

TEST_CASE("the I piece gets wider kick candidates than the rest") {
  REQUIRE(kicks_for(Tetromino::I).size() == 5);
  REQUIRE(kicks_for(Tetromino::T).size() == 3);
  // Trying the unshifted rotation first is what keeps rotation in place when
  // there is already room for it.
  REQUIRE(kicks_for(Tetromino::T)[0] == Offset{0, 0});
  REQUIRE(kicks_for(Tetromino::I)[0] == Offset{0, 0});
}
