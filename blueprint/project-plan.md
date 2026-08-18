# Project Plan

## 1. Problem - What problem are we solving?

Stephen Brennan's Tetris is ~500 lines of C whose engine is entangled with its ncurses front end, which makes the game logic hard to test in isolation.
This project reimplements it in C++20 with that separation enforced by the build system rather than by convention, as a hands-on exercise in modern C++ and clean module boundaries.
See [PLAN.md](../PLAN.md) for the full design reasoning and the deliberate deviations from the original.

## 2. Users - Who is this for?

Primarily the author: this is a personal learning exercise in C++20, CMake, and testable architecture.
Secondarily, anyone reading the repo as a worked reference for engine/UI separation, or playing the finished terminal game.

## 3. Features - What does the MVP need?

Everything below is already shipped.

- Core value types and constexpr tables: `Cell`, `Tetromino`, `Offset`, `Move`, tetromino shapes, gravity intervals, wall-kick offsets.
- `Piece` with value semantics: `cells()`, `rotated()`, `translated()` return candidates rather than mutating.
- `Board` holding settled cells only, with `fits()` as a pure query and `clear_full_lines()`.
- `Game` as the entire public API: spawn, gravity, movement, hard drop, lock, respawn, game over.
- Rotation with wall kicks driven by a table.
- Scoring (`{0, 40, 100, 300, 1200}` scaled by `level + 1`) and level progression to a cap of 19.
- 7-bag randomiser, next preview, and hold with a once-per-piece lockout.
- Versioned save and load, including RNG state and bag contents, returning `std::optional<Game>` on failure.
- ncurses front end: RAII `Terminal`, `Renderer`, fixed-timestep main loop, pause menu, boss mode, colour per tetromino.
- Catch2 test suite covering the engine through its public API only.

## 4. Data - What are we storing?

No database and no user accounts.
The only persisted artifact is `tetris.save`, an explicit versioned text snapshot in the working directory: a magic string, format version, board dimensions, score, level, lines remaining and cleared, gravity counter, game-over and hold-lockout flags, bag contents, RNG state, the flat board cells, the three piece slots, and an `END` terminator.

## 5. Tech - What stack are we using?

- C++20, no external runtime dependencies in the engine.
- CMake 3.20+ with three targets: `tetris_core` (static library, no UI), `tetris` (executable, core + ncurses), `tetris_tests` (core only).
- ncurses for the terminal front end, found via `find_package(Curses)`.
- Catch2 v3.5.2, fetched at configure time via `FetchContent`.
- Warnings as errors everywhere: `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`.

## 6. Monetize - How will this make money?

It doesn't.
This is a learning exercise, not a commercial product.

## 7. UI/UX - How should this look and feel?

A terminal game that fits an 80x24 screen, board included, with a coloured cell per tetromino type.
Next and hold panels sit beside the board; the hold panel greys out while the once-per-piece lockout is in force so the rule is visible rather than mysterious.
Controls follow both arrow-key and vi-style conventions.
Terminal state is restored on every exit path, including unexpected ones, via RAII.

## 8. Deployment - Where and how will this ship?

Not deployed.
It builds and runs locally from source (`cmake -S . -B build && cmake --build build`, then `./build/tetris`).

> TODO (confirm) - no install target, packaging, or CI workflow exists today. Add them only if distribution becomes a goal.
