#include "tetris/game.hpp"

#include <algorithm>
#include <ostream>
#include <utility>

#include "tetris/tables.hpp"

namespace tetris {

Game::Game(int rows, int cols)
    : board_(rows, cols),
      // Placeholders. Both slots are overwritten before the constructor
      // returns, because rng_ is declared after them and so is not usable in
      // the initialiser list.
      falling_(Tetromino::I, {0, 0}),
      next_(Tetromino::I, {0, 0}),
      rng_(std::mt19937::default_seed) {}

Game::Game(Board board, std::uint32_t seed) : Game(board.rows(), board.cols()) {
  board_ = std::move(board);
  rng_.seed(seed);
  falling_ = spawn(draw_from_bag());
  next_ = spawn(draw_from_bag());
  reset_gravity();
  if (!board_.fits(falling_)) game_over_ = true;
}

Game::Game(int rows, int cols, std::uint32_t seed)
    : Game(Board(rows, cols), seed) {}

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

Cell Game::at(int row, int col) const {
  for (const Offset& c : falling_.cells()) {
    if (c.row == row && c.col == col) return falling_.cell();
  }
  return board_.at(row, col);
}

Piece Game::landing_position() const {
  Piece piece = falling_;
  while (true) {
    const Piece lower = piece.translated({1, 0});
    if (!board_.fits(lower)) return piece;
    piece = lower;
  }
}

void Game::print(std::ostream& out) const {
  for (int row = 0; row < rows(); ++row) {
    out << '|';
    for (int col = 0; col < cols(); ++col) out << to_char(at(row, col));
    out << "|\n";
  }
  out << "score " << points_ << "  level " << level_ << "  lines to next "
      << lines_remaining_ << '\n';
}

// ---------------------------------------------------------------------------
// Ticking
// ---------------------------------------------------------------------------

TickResult Game::tick(Move move) {
  if (game_over_) return TickResult::GameOver;

  // A move that locks a piece consumes the whole tick, so the piece that
  // respawns behind it gets its full gravity interval rather than arriving
  // one tick into it.
  const bool locked = apply_move(move);
  if (!locked && !game_over_) gravity_tick();

  return game_over_ ? TickResult::GameOver : TickResult::Continue;
}

// Returns true when the move locked the falling piece.
bool Game::apply_move(Move move) {
  switch (move) {
    case Move::Left: try_move({0, -1}); break;
    case Move::Right: try_move({0, 1}); break;
    case Move::RotateCW: try_rotate(true); break;
    case Move::RotateCCW: try_rotate(false); break;
    case Move::Hold: hold(); break;
    case Move::None: break;
    case Move::Drop:
      hard_drop();
      return true;
  }
  return false;
}

void Game::try_move(Offset delta) {
  const Piece candidate = falling_.translated(delta);
  if (board_.fits(candidate)) falling_ = candidate;
}

bool Game::try_rotate(bool clockwise) {
  const Piece rotated = falling_.rotated(clockwise);
  // Walk the kick candidates in order and commit to the first that fits. The
  // first entry is the null offset, so a rotation with room turns in place.
  for (const Offset& kick : kicks_for(falling_.type())) {
    const Piece candidate = rotated.translated(kick);
    if (board_.fits(candidate)) {
      falling_ = candidate;
      return true;
    }
  }
  return false;
}

void Game::hard_drop() {
  falling_ = landing_position();
  lock_and_respawn();
}

void Game::gravity_tick() {
  if (--ticks_till_gravity_ > 0) return;

  const Piece lower = falling_.translated({1, 0});
  if (board_.fits(lower)) {
    falling_ = lower;
    reset_gravity();
  } else {
    lock_and_respawn();
  }
}

void Game::lock_and_respawn() {
  board_.lock(falling_);
  clear_lines_and_score();

  // The one and only place the hold lockout is released. Moving this into a
  // helper shared with hold() would quietly restore unlimited swapping.
  hold_used_this_piece_ = false;

  falling_ = next_;
  next_ = spawn(draw_from_bag());
  reset_gravity();

  if (!board_.fits(falling_)) game_over_ = true;
}

void Game::clear_lines_and_score() {
  const int cleared = board_.clear_full_lines();
  if (cleared == 0) return;

  lines_cleared_ += cleared;
  points_ += kLineMultiplier[static_cast<std::size_t>(cleared)] * (level_ + 1);
  lines_remaining_ -= cleared;

  if (lines_remaining_ <= 0) {
    const int overshoot = -lines_remaining_;
    level_ = std::min(kMaxLevel, level_ + 1 + overshoot / kLinesPerLevel);
    lines_remaining_ = kLinesPerLevel - overshoot % kLinesPerLevel;
  }
}

// ---------------------------------------------------------------------------
// Hold
// ---------------------------------------------------------------------------

void Game::hold() {
  if (hold_used_this_piece_) return;
  hold_used_this_piece_ = true;

  if (held_) {
    const Piece swapped_in = spawn(held_->type());
    held_ = spawn(falling_.type());
    falling_ = swapped_in;
  } else {
    held_ = spawn(falling_.type());
    falling_ = next_;
    next_ = spawn(draw_from_bag());
  }

  reset_gravity();
  if (!board_.fits(falling_)) game_over_ = true;
}

// ---------------------------------------------------------------------------
// Pieces and gravity
// ---------------------------------------------------------------------------

Piece Game::spawn(Tetromino type) const {
  return Piece(type, {0, cols() / 2 - 2});
}

Tetromino Game::draw_from_bag() {
  if (bag_.empty()) {
    bag_.reserve(kTetrominoCount);
    for (int i = 0; i < kTetrominoCount; ++i) {
      bag_.push_back(static_cast<Tetromino>(i));
    }
    std::shuffle(bag_.begin(), bag_.end(), rng_);
  }
  const Tetromino drawn = bag_.back();
  bag_.pop_back();
  return drawn;
}

void Game::reset_gravity() {
  ticks_till_gravity_ = kGravity[static_cast<std::size_t>(level_)];
}

bool operator==(const Game& a, const Game& b) {
  return a.board_ == b.board_ && a.falling_ == b.falling_ &&
         a.next_ == b.next_ && a.held_ == b.held_ &&
         a.hold_used_this_piece_ == b.hold_used_this_piece_ &&
         a.points_ == b.points_ && a.level_ == b.level_ &&
         a.lines_remaining_ == b.lines_remaining_ &&
         a.lines_cleared_ == b.lines_cleared_ &&
         a.ticks_till_gravity_ == b.ticks_till_gravity_ &&
         a.game_over_ == b.game_over_ && a.rng_ == b.rng_ && a.bag_ == b.bag_;
}

}  // namespace tetris
