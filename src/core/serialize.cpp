// Explicit, versioned serialisation.
//
// The original memcpy'd the game struct straight to disk. That was already
// fragile in C, and here it would be undefined behaviour: Game owns a vector,
// an optional, and a mt19937. So the format is written field by field, in
// text, behind a magic string and a version number. A file from an older
// build, a truncated file, or a file that is not a save at all all come back
// as an empty optional rather than a crash or a corrupt game.

#include <istream>
#include <limits>
#include <ostream>
#include <string>

#include "tetris/game.hpp"
#include "tetris/tables.hpp"

namespace tetris {
namespace {

constexpr const char* kMagic = "TETRIS-SAVE";
// Version 2 added the lock delay counters; a v1 file fails the load cleanly.
constexpr int kFormatVersion = 2;

// Without a terminator, a file truncated inside its last field still parses:
// the reader takes the digits it can see and stops happily. The sentinel
// turns every truncation into a failed load.
constexpr const char* kTerminator = "END";

// A board bigger than this is a corrupt file rather than an ambitious player.
constexpr int kMaxDimension = 4096;

void write_piece(std::ostream& out, const Piece& piece) {
  out << static_cast<int>(piece.type()) << ' ' << piece.orientation() << ' '
      << piece.origin().row << ' ' << piece.origin().col << '\n';
}

// Every read goes through a checked helper, so a malformed field fails the
// load instead of leaving a half-built game behind.
bool read_int(std::istream& in, int& value, int low, int high) {
  if (!(in >> value)) return false;
  return value >= low && value <= high;
}

bool read_piece(std::istream& in, std::optional<Piece>& piece) {
  int type = 0;
  int orientation = 0;
  int row = 0;
  int col = 0;
  if (!read_int(in, type, 0, kTetrominoCount - 1)) return false;
  if (!read_int(in, orientation, 0, kOrientations - 1)) return false;
  if (!read_int(in, row, -kMaxDimension, kMaxDimension)) return false;
  if (!read_int(in, col, -kMaxDimension, kMaxDimension)) return false;
  piece = Piece(static_cast<Tetromino>(type), {row, col}, orientation);
  return true;
}

}  // namespace

void Game::save(std::ostream& out) const {
  out << kMagic << '\n' << kFormatVersion << '\n';
  out << board_.rows() << ' ' << board_.cols() << '\n';
  out << points_ << ' ' << level_ << ' ' << lines_remaining_ << ' '
      << lines_cleared_ << ' ' << ticks_till_gravity_ << ' '
      << (game_over_ ? 1 : 0) << ' ' << (hold_used_this_piece_ ? 1 : 0) << '\n';
  out << lock_ticks_ << ' ' << lock_resets_ << '\n';

  // The bag and the generator both have to travel, or the upcoming piece
  // sequence changes across a save and load.
  out << bag_.size();
  for (Tetromino type : bag_) out << ' ' << static_cast<int>(type);
  out << '\n' << rng_ << '\n';

  write_piece(out, falling_);
  write_piece(out, next_);
  out << (held_ ? 1 : 0) << '\n';
  if (held_) write_piece(out, *held_);

  for (int row = 0; row < board_.rows(); ++row) {
    for (int col = 0; col < board_.cols(); ++col) {
      out << static_cast<int>(board_.at(row, col)) << ' ';
    }
    out << '\n';
  }

  out << kTerminator << '\n';
}

std::optional<Game> Game::load(std::istream& in) {
  std::string magic;
  int version = 0;
  if (!(in >> magic) || magic != kMagic) return std::nullopt;
  if (!read_int(in, version, kFormatVersion, kFormatVersion)) {
    return std::nullopt;
  }

  int rows = 0;
  int cols = 0;
  if (!read_int(in, rows, 1, kMaxDimension)) return std::nullopt;
  if (!read_int(in, cols, 1, kMaxDimension)) return std::nullopt;

  Game game(rows, cols);

  int game_over = 0;
  int hold_used = 0;
  if (!read_int(in, game.points_, 0, std::numeric_limits<int>::max())) {
    return std::nullopt;
  }
  if (!read_int(in, game.level_, 0, kMaxLevel)) return std::nullopt;
  if (!read_int(in, game.lines_remaining_, 1, kLinesPerLevel)) {
    return std::nullopt;
  }
  if (!read_int(in, game.lines_cleared_, 0, std::numeric_limits<int>::max())) {
    return std::nullopt;
  }
  if (!read_int(in, game.ticks_till_gravity_, 0, kGravity[0])) {
    return std::nullopt;
  }
  if (!read_int(in, game_over, 0, 1)) return std::nullopt;
  if (!read_int(in, hold_used, 0, 1)) return std::nullopt;
  game.game_over_ = game_over == 1;
  game.hold_used_this_piece_ = hold_used == 1;

  if (!read_int(in, game.lock_ticks_, -1, kLockDelayTicks)) {
    return std::nullopt;
  }
  if (!read_int(in, game.lock_resets_, 0, kMaxLockResets)) {
    return std::nullopt;
  }

  int bag_size = 0;
  if (!read_int(in, bag_size, 0, kTetrominoCount)) return std::nullopt;
  game.bag_.clear();
  for (int i = 0; i < bag_size; ++i) {
    int type = 0;
    if (!read_int(in, type, 0, kTetrominoCount - 1)) return std::nullopt;
    game.bag_.push_back(static_cast<Tetromino>(type));
  }

  if (!(in >> game.rng_)) return std::nullopt;

  std::optional<Piece> piece;
  if (!read_piece(in, piece)) return std::nullopt;
  game.falling_ = *piece;
  if (!read_piece(in, piece)) return std::nullopt;
  game.next_ = *piece;

  int has_held = 0;
  if (!read_int(in, has_held, 0, 1)) return std::nullopt;
  if (has_held == 1) {
    if (!read_piece(in, piece)) return std::nullopt;
    game.held_ = piece;
  } else {
    game.held_.reset();
  }

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      int cell = 0;
      if (!read_int(in, cell, 0, kTetrominoCount)) return std::nullopt;
      game.board_.set(row, col, static_cast<Cell>(cell));
    }
  }

  std::string terminator;
  if (!(in >> terminator) || terminator != kTerminator) return std::nullopt;

  return game;
}

}  // namespace tetris
