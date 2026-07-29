#include "tetris/piece.hpp"

#include <cassert>

namespace tetris {

Piece::Piece(Tetromino type, Offset origin, int orientation)
    : type_(type), orientation_(orientation), origin_(origin) {
  assert(orientation >= 0 && orientation < kOrientations);
}

std::array<Offset, kCellsPerPiece> Piece::cells() const {
  const auto& shape = kTetrominos[static_cast<std::size_t>(type_)]
                                 [static_cast<std::size_t>(orientation_)];
  std::array<Offset, kCellsPerPiece> out{};
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = origin_ + shape[i];
  return out;
}

Piece Piece::rotated(bool clockwise) const {
  const int delta = clockwise ? 1 : kOrientations - 1;
  return Piece(type_, origin_, (orientation_ + delta) % kOrientations);
}

Piece Piece::translated(Offset delta) const {
  return Piece(type_, origin_ + delta, orientation_);
}

Piece Piece::with_origin(Offset origin) const {
  return Piece(type_, origin, orientation_);
}

}  // namespace tetris
