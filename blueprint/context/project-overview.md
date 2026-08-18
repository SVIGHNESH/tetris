# Tetris - Project Overview

> A C++20 reimplementation of Stephen Brennan's Tetris, with the engine strictly separated from the terminal so the whole game is testable without ncurses.

_Generated from `blueprint/project-plan.md` and `blueprint/build-plan.md`. Don't hand-edit; re-run `/overview` when the plans change._

## Problem

The original is roughly 500 lines of C whose game logic and ncurses front end are entangled: the falling piece is stamped directly into the board array, so every move test has to remove and replace it, and nothing can be exercised without a terminal.
This project rebuilds it in C++20 with that boundary enforced by the build system rather than by convention.
The point is the exercise itself - practising modern C++, CMake, and a module seam that a test can sit behind.

Full design reasoning, including the three deliberate deviations from the original, lives in [PLAN.md](../../PLAN.md).

## Users

- **The author** - the primary user. This is a learning exercise, so the value is in building and maintaining it.
- **Developers reading the repo** - a worked reference for engine/UI separation in C++.
- **Anyone playing it** - a terminal Tetris on Linux or macOS. No accounts, no tiers, no network.

## Features

All ten are shipped; the build plan carries no unchecked items. Order follows [PLAN.md](../../PLAN.md) section 13.

1. **Scaffold** - CMake with C++20, three targets, warnings as errors, Catch2 via `FetchContent`.
2. **Types and tables** - `Cell`, `Tetromino`, `Offset`, `Move`, plus constexpr tetromino, gravity, and kick tables.
3. **Piece** - value-semantic `cells()`, `rotated()`, `translated()`; candidates are tested before they are committed.
4. **Board** - settled cells only, so `fits()` is a pure query with no mutation.
5. **Game skeleton** - spawn, gravity, lateral movement, hard drop, lock, respawn, game over, debug `print()`.
6. **Rotation with wall kicks** - table-driven candidate offsets, first fit wins.
7. **Scoring and levels** - the points table scaled by level, 10 lines per level, cap at 19.
8. **Hold and next** - 7-bag draw, next preview, hold with a once-per-piece lockout. **The headline mechanic**, and the one with the subtlest invariant.
9. **ncurses UI** - RAII `Terminal`, `Renderer`, fixed-timestep main loop, a colour per tetromino.
10. **Extras** - pause menu, boss mode, versioned save and load, warning cleanup, full test coverage.

11. **Soft drop** - the down key accelerates the fall by one row per tick rather than hard dropping; space keeps the instant drop.
12. **Ghost piece** - the renderer draws the landing position as a dim `::` marker.
13. **Lock delay** - a half-second grace window after touchdown, restarted by movement up to 15 times per piece; save format bumped to version 2.
14. **Drop scoring** - 1 point per soft-dropped row, 2 per hard-dropped row, flat, on top of the clear-points table.

## Data model

No database and no user accounts.
The only persisted artifact is the save file.

### Save file (`tetris.save`)

Written by `Game::save(std::ostream&)`, read by `Game::load(std::istream&)`.
Plain text, field by field, so a truncated or stale file fails cleanly rather than corrupting a game.

- `magic` (string) - `TETRIS-SAVE`
- `version` (int) - currently `2` (lock delay counters added); a mismatch fails the load
- `rows`, `cols` (int) - board dimensions, rejected above 4096 as corrupt
- `points`, `level`, `lines_remaining`, `lines_cleared`, `ticks_till_gravity` (int) - progress state
- `game_over`, `hold_used_this_piece` (0/1) - flags
- `bag` (count, then that many tetromino ids) - the remainder of the current 7-bag
- `rng` (`std::mt19937` stream state) - the generator itself
- `cells` (rows x cols of `Cell` ids) - settled board, row-major
- `falling`, `next`, `held` (type, orientation, row, col each; `held` optional)
- `END` (string) - terminator

> Locked, and load-bearing: **the bag and the RNG state both travel.** Serialising one without the other makes the upcoming piece sequence merely plausible instead of identical across a round trip, and `test_serialize.cpp` asserts against exactly that.

> Locked: **`load` returns `std::optional<Game>`.** A corrupt or stale file is a normal failure the UI reports, not a crash. Bumping the format is a version bump, not a silent field addition.

## Tech stack

- **C++20** - no compiler extensions; the engine has no external runtime dependencies.
- **CMake 3.20+** - three targets: `tetris_core` (static lib, no UI), `tetris` (core + ncurses), `tetris_tests` (core only). The dependency direction is enforced here, not by convention.
- **ncurses** - the terminal front end, found via `find_package(Curses)`. The only target that links it is `tetris`.
- **Catch2 v3.5.2** - fetched at configure time; the first configure needs network access.
- **Warnings as errors** - `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow` on every target. There is no separate lint step because the build is the lint.

## Monetization

Not in v1, and not planned. This is a learning exercise.

## UI/UX

A terminal game sized to fit 80x24, board and hint line included, with a colour per tetromino type.
There are no routes; there are screens.

- **Board** - the field with the falling piece composited in, next and hold panels beside it, score and level.
- **Hold panel** - greys out while the once-per-piece lockout is in force, so the rule is visible rather than mysterious.
- **Pause menu** (`p`, escape) - save, load, boss mode, restart, quit.
- **Boss mode** (`b`) - a fake terminal screen.

Controls accept both arrow keys and vi-style bindings; see the table in [README.md](../../README.md).
Terminal state is restored on every exit path, expected or not, via RAII - the single biggest robustness win over the original.

## Deployment

Not deployed. Built and run locally from source:

```sh
cmake -S . -B build
cmake --build build
./build/tetris
```

`-DTETRIS_BUILD_UI=OFF` builds the engine and tests without ncurses; `-DTETRIS_BUILD_TESTS=OFF` skips Catch2.

> TODO - no install target, packaging, or release binary exists. Add one only if distribution becomes a goal.

## Testing

The test gate is **on**. `AGENTS.md` declares `ctest --test-dir build`, currently 67 tests, all passing.
A step that adds engine logic ships a passing test in the same diff; UI steps are verified by running the binary.
Tests link `tetris_core` only and must never need a terminal - that constraint is the whole point of the architecture, so protect it.

## Open questions

> - **Deployment/packaging is undecided** (see above). Not a blocker while this stays local.
