# Tetris in C++ - Implementation Plan

A C++20 reimplementation of Stephen Brennan's Tetris.

Reference article: https://brennan.io/2015/06/12/tetris-reimplementation/
Reference source: https://github.com/brenns10/tetris

## 1. What we are porting

The original is roughly 500 lines of C in `tetris.c` plus an ncurses front end in `main.c`.
The design idea worth preserving is the strict separation between the two: `tetris.c` never includes ncurses, and `main.c` never touches the game struct's internals beyond what `tetris.h` exposes.
That is what makes the engine reusable and, more importantly, testable.

The original public surface is small.
`tg_create` / `tg_delete` for lifetime, `tg_load` / `tg_save` for persistence, `tg_get` and `tg_check` for reading the board, `tg_tick(game, move)` to advance one frame, and `tg_print` for debug output.
Everything else (gravity, wall kicks, line clearing, scoring, level progression) is private to the translation unit.

Two extern tables drive the whole game.
`TETROMINOS[7][4][4]` maps type and orientation to four cell offsets from a piece origin.
`GRAVITY_LEVEL[20]` gives ticks per gravity step for each level, decreasing as the level rises.

## 2. Locked decisions

These were settled up front and the rest of the plan assumes them.

**Board dimensions are runtime values, backed by `std::vector`.**
The original allocates `rows * cols` chars, and there is no measurable cost to keeping that flexibility.
A fixed compile-time 20x10 `std::array` would buy nothing here and would throw away the "any board size" feature.

**Piece randomizer is a 7-bag shuffle, not uniform random.**
The original calls `random()` for each piece, which permits long droughts of any single tetromino.
A 7-bag shuffles all seven types, deals them out, then shuffles a fresh bag.
This is what modern Tetris does and it is about eight lines of code.
It is a deliberate deviation from the original and it does change how the game feels.

**Hold is once per piece.**
Using hold locks the slot until the current piece locks into the stack.
The original allows unlimited swaps, which has two problems: each swap respawns the piece at the top, so repeatedly pressing hold stalls the game forever, and a free toggle removes the tactical cost that makes the mechanic interesting.
Once-per-piece also matches what any player coming from another Tetris will expect.

Implementation note for the hold rule.
`hold_used_this_piece_` is set to `true` inside `apply_move(Move::Hold)` and cleared only in `lock_and_respawn()`.
The flag clear must live on the lock path specifically, not inside whatever shared helper both the hold path and the lock path call to install a new falling piece.
Putting it in the shared helper silently reverts the behavior to unlimited swaps.

## 3. Project layout

```
tetris/
  CMakeLists.txt
  PLAN.md
  include/tetris/
    cell.hpp          # enum class Cell, Tetromino, conversions
    location.hpp      # Offset value type
    piece.hpp         # Piece: type + orientation + position
    board.hpp         # Board: settled cells only
    move.hpp          # enum class Move
    tables.hpp        # constexpr tetromino, gravity, and kick tables
    game.hpp          # class Game - the entire public API
  src/core/
    board.cpp
    piece.cpp
    game.cpp
    serialize.cpp
  src/ui/
    main.cpp
    terminal.hpp / terminal.cpp
    renderer.hpp / renderer.cpp
    boss_mode.cpp
  tests/
    test_piece.cpp
    test_board.cpp
    test_game.cpp
    test_scoring.cpp
```

Three CMake targets.
`tetris_core` is a static library with zero UI dependencies and no ncurses anywhere in its include path.
`tetris` is the executable, linking core plus ncurses.
`tetris_tests` links core only.

The dependency direction is enforced by the build system rather than by convention.
If someone includes `<ncurses.h>` from `src/core/`, the build breaks.

## 4. Core types

```cpp
enum class Cell : std::uint8_t { Empty, I, J, L, O, S, T, Z };
enum class Tetromino : std::uint8_t { I, J, L, O, S, T, Z };
enum class Move { Left, Right, RotateCW, RotateCCW, Drop, Hold, None };

struct Offset { int row, col; };
```

`Cell::Empty` is deliberately zero so a default-constructed board is empty.
That reproduces the original's `TYPE_TO_CELL` trick without a macro.
A `constexpr Cell to_cell(Tetromino)` handles the conversion.

`Offset` is signed and used for both absolute positions and deltas, matching the original `tetris_location`.

## 5. Tables become constexpr

The original declares `extern tetris_location TETROMINOS[...]` and defines it in the `.c` file.
In C++ these become `constexpr std::array` in `tables.hpp`:

```cpp
inline constexpr std::array<std::array<std::array<Offset, 4>, 4>, 7> kTetrominos = {...};
inline constexpr std::array<int, kMaxLevel + 1> kGravity = {...};
```

No mutable global state, no static initialization order concerns, and the compiler can fold lookups at compile time.

Constants: `kTetrominoCount = 7`, `kOrientations = 4`, `kCellsPerPiece = 4`, `kMaxLevel = 19`, `kLinesPerLevel = 10`.

## 6. Piece owns its own geometry

In the original, callers index `TETROMINOS[b.typ][b.ori][i]` and add `b.loc` by hand at every call site.
Pulling that into the type is the first real improvement.

```cpp
class Piece {
public:
  Piece(Tetromino type, Offset origin);

  std::array<Offset, 4> cells() const;   // absolute board coordinates
  Piece rotated(bool clockwise) const;   // returns a candidate, does not mutate
  Piece translated(Offset delta) const;

  Tetromino type() const;
  Cell cell() const;
  int orientation() const;
  Offset origin() const;

private:
  Tetromino type_;
  int orientation_;
  Offset origin_;
};
```

Value semantics matter here.
Rotation and translation return a candidate that the caller tests before committing.
That eliminates the entire class of "undo the move I just made" bugs that comes with mutating in place and rolling back on failure.

## 7. Board holds settled cells only

This is the largest structural change from the original.

The C version stamps the falling piece directly into the board array.
As a result every move test has to `remove()` the piece first and `put()` it back afterwards, and every private function is entangled with maintaining that invariant.

Here the board stores only locked cells.
Collision testing is a pure query with no mutation at all.

```cpp
class Board {
public:
  Board(int rows, int cols);

  Cell at(int row, int col) const;
  bool in_bounds(int row, int col) const;
  bool fits(const Piece&) const;         // pure: bounds + occupancy, no side effects
  void lock(const Piece&);
  int clear_full_lines();                // returns how many rows were cleared

  int rows() const;
  int cols() const;

private:
  int rows_, cols_;
  std::vector<Cell> cells_;              // row-major, flat
};
```

The falling piece is composited only at read time.
`Game::at(row, col)` overlays the falling piece on top of `board.at()` for the renderer.
The UI sees exactly what it saw before, and the engine gets a far simpler internal contract.

## 8. Game is the entire public API

```cpp
enum class TickResult { Continue, GameOver };

class Game {
public:
  Game(int rows, int cols, std::uint32_t seed);

  TickResult tick(Move move);

  Cell at(int row, int col) const;       // board with falling piece composited in
  int score() const;
  int level() const;
  int lines_remaining() const;
  const Piece& falling() const;
  const Piece& next() const;
  std::optional<Piece> held() const;
  bool can_hold() const;

  void save(std::ostream&) const;
  static std::optional<Game> load(std::istream&);

private:
  void apply_move(Move);
  void gravity_tick();
  bool try_rotate(bool clockwise);       // includes wall kicks
  void lock_and_respawn();               // the only place hold_used_this_piece_ is cleared
  void clear_lines_and_score();
  Tetromino draw_from_bag();

  Board board_;
  Piece falling_, next_;
  std::optional<Piece> held_;
  bool hold_used_this_piece_ = false;
  int points_ = 0;
  int level_ = 0;
  int lines_remaining_ = kLinesPerLevel;
  int ticks_till_gravity_;
  std::mt19937 rng_;
  std::vector<Tetromino> bag_;
};
```

`tick()` keeps the original's five-step body.
Apply the input move, run the gravity counter, on lock detect and clear full lines, adjust score and level, then report whether the game is over.

Returning a `TickResult` enum rather than a bare `bool` leaves room to add `Locked` or `LevelUp` later so the UI can react with sound or a flash without the engine learning about either.

## 9. Rotation and wall kicks

The original does the simple thing.
Rotate in place, and if the result does not fit, try shifting one column left, then one right, then two columns each way for the I piece.

Keep that behavior so the game feels the same, but express the candidate offsets as a `constexpr` table that `try_rotate` walks in order, committing to the first offset that fits.
Swapping in full SRS kick tables later then becomes a data change rather than a code change.

## 10. RNG and the piece sequence

`std::mt19937` is a member seeded from the constructor, so a given seed produces a reproducible game.
That reproducibility is what makes real regression tests possible.

The 7-bag works as follows.
`bag_` holds the remaining types for the current bag.
When it is empty, refill it with all seven tetrominoes and `std::shuffle` with `rng_`.
`draw_from_bag()` pops the back element.

The bag contents and the RNG state both have to be serialized, or a save/load round trip changes the upcoming piece sequence.

## 11. Save and load

Do not memcpy the struct.
The original's approach was already fragile in C, and with `std::vector` and `std::optional` members it is undefined behavior.

Write an explicit versioned serializer.
Magic bytes, then a format version, then board dimensions, score, level, lines remaining, gravity counter, RNG state, bag contents, `hold_used_this_piece_`, the flat board cells, and the three piece slots.

`load` returns `std::optional<Game>` so a corrupt file or a stale format version is a normal failure the UI can report, not a crash.

## 12. UI layer

Keep ncurses.
It is the right tool for drawing a grid of cells in a terminal and it is perfectly usable from C++.

**`Terminal`** is an RAII wrapper.
The constructor does `initscr`, `cbreak`, `noecho`, `keypad`, `curs_set(0)`, and color pair setup.
The destructor does `endwin`.
This is the single biggest robustness win over the original, which leaves the user's terminal wrecked if the process exits on an unexpected path.

**`Renderer`** takes a `const Game&` and draws the board, the next preview, the hold slot, the score, and the level.
It reads only through the public API.
The hold slot should render differently when `can_hold()` is false, so the once-per-piece lockout is visible rather than mysterious.

**Main loop** uses a fixed timestep driven by `std::chrono::steady_clock` with an accumulator, and non-blocking input via `timeout(0)`.
Input events map to `Move` values and feed the next tick.
This decouples frame rate from tick rate, which the original's `timeout()`-based loop conflates.

Pause menu, boss mode (the fake terminal screen), and the save/load prompts all live in the UI layer.
The engine never learns that any of them exist.

## 13. Build order

Steps 2 through 8 are all fully testable without a terminal.
That is the payoff of carrying the original's separation into C++.

1. **Scaffold.**
   CMake with C++20, the three targets, `-Wall -Wextra -Wpedantic -Werror`, and Catch2 wired up via `FetchContent`.

2. **Types and tables.**
   `Cell`, `Tetromino`, `Offset`, `Move`, plus the tetromino and gravity tables.
   Transcribe the seven shapes across four orientations carefully; this is the most error-prone data entry in the project.
   First test: every shape in every orientation has exactly four distinct cells.

3. **Piece.**
   `cells()`, `rotated()`, `translated()`.
   Test that four rotations return to the starting orientation, and that O is unchanged by rotation.

4. **Board.**
   Construction, `at`, `in_bounds`, `fits`, `lock`, `clear_full_lines`.
   Test multi-line clears and verify rows above a cleared line shift down correctly.

5. **Game skeleton.**
   Spawn, gravity, left and right movement, lock, respawn, and game over when a spawned piece does not fit.
   Playable at this point through a debug `print()` to stdout, before any ncurses exists.

6. **Rotation with kicks.**
   Test rotating an I piece flush against each wall and inside a one-wide well.

7. **Scoring and levels.**
   Points table `{0, 40, 100, 300, 1200}` scaled by `level + 1`.
   Every 10 lines advances a level up to 19, and each level pulls its gravity interval from `kGravity`.

8. **Hold and next.**
   7-bag draw, next preview, hold with the once-per-piece lockout.
   Test that a second hold on the same piece is rejected, and that the lockout clears after the piece locks.

9. **ncurses UI.**
   `Terminal`, `Renderer`, the main loop, and a color per tetromino.

10. **Extras.**
    Pause menu, boss mode, save and load, then a cleanup pass for warnings and remaining test coverage.

## 14. Test coverage targets

- Every tetromino orientation has exactly four distinct cells.
- Four rotations are the identity; O is rotation-invariant.
- Single, double, triple, and tetris line clears, including non-adjacent cleared rows.
- Wall kick succeeds against the left wall, the right wall, and inside a well.
- Wall kick correctly fails when there is genuinely no room.
- Score matches the points table at several levels.
- Level advances at exactly 10 lines and caps at 19.
- Gravity interval shortens as the level rises.
- Hold on an empty slot pulls from the bag; hold on a full slot swaps.
- A second hold on the same piece is rejected.
- The hold lockout clears after the piece locks, and not after a hold.
- Game over fires when a spawned piece does not fit.
- A 7-bag never deals the same type twice before dealing all seven.
- Save then load reproduces an identical game, including the upcoming piece sequence.
