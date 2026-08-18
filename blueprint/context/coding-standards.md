# Coding Standards

The conventions this project actually follows, read off the existing code.
[PLAN.md](../../PLAN.md) holds the design reasoning behind them.

## C++

- C++20, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- The build is warnings-as-errors: `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`.
  Silence a warning by fixing the code, not by suppressing it.
- Explicit `static_cast` at every narrowing or signedness boundary; `-Wconversion` will catch anything implicit.
- Prefer value semantics for small types.
  `Piece::rotated()` and `Piece::translated()` return a candidate the caller tests before committing, rather than mutating and rolling back on failure.
- Tables are `inline constexpr std::array` in headers, never mutable globals or `extern` arrays.
- `std::optional` for operations that can fail normally (`Game::load`), not exceptions or sentinel values.
- `assert` for internal invariants the caller is contractually required to satisfy (bounds already checked by `in_bounds`).
- RAII for anything that must be undone on every exit path, including unexpected ones (`Terminal` wraps `initscr`/`endwin`).
- Standard algorithms over hand-rolled loops where they read at least as clearly (`std::copy_n`, `std::fill_n`, `std::shuffle`).

## Architecture

The one rule that outranks the rest: **`tetris_core` never depends on the UI.**

- `tetris_core` (`src/core/`, `include/tetris/`) links no ncurses and has no ncurses on its include path.
  The dependency direction is enforced by CMake, so a `#include <ncurses.h>` under `src/core/` breaks the build rather than passing review.
- `tetris` (`src/ui/`) is the only target that knows a terminal exists.
  Pause menu, boss mode, save/load prompts, and input mapping all live here.
- The UI reads the engine only through `Game`'s public API.
  If the renderer needs something, add an accessor to `Game`; do not reach into internals or move UI concepts into the engine.
- `Game` is the entire public engine surface.
  `Board` and `Piece` are public types it composes, but gravity, kicks, scoring, and the bag stay private to `Game`.
- `Board` stores settled cells only.
  The falling piece is composited at read time in `Game::at()`, so collision testing (`Board::fits`) is a pure query with no mutation.

## File organization

```
include/tetris/   public engine headers, one type per header
src/core/         engine implementation (piece, board, game, serialize)
src/ui/           ncurses front end (main, terminal, renderer, boss_mode)
tests/            Catch2 suites, one per engine area, plus support.hpp
```

- Public headers live in `include/tetris/` and are included as `"tetris/board.hpp"`.
- UI headers sit next to their `.cpp` in `src/ui/` and are private to that target.
- `#pragma once`, not include guards.
- Include order: the matching header first, then standard library, then project headers, each group separated by a blank line.

## Naming

- Everything in the engine is in `namespace tetris`, closed with `}  // namespace tetris`.
- Types: `PascalCase` (`Game`, `TickResult`).
- Functions, variables, parameters: `snake_case`.
- Private data members: trailing underscore (`board_`, `hold_used_this_piece_`).
- Constants and constexpr tables: `k` prefix (`kMaxLevel`, `kTetrominos`, `kLinesPerLevel`).
- Enumerators: `PascalCase` inside `enum class` (`Cell::Empty`, `Move::RotateCW`).

## Formatting

- Two-space indent, no tabs.
- Braces on the same line; `public:` / `private:` indented one space.
- Single-statement bodies may stay on one line when short (`if (!in_bounds(...)) return false;`).
- Constructor initialiser lists broken one member per line, aligned under the colon.

## Testing

The test gate is **on**: `AGENTS.md` declares a `test` command, so a step that adds engine logic ships a passing test in the same reviewable diff.

- Runner is Catch2 v3, fetched at configure time.
  Run via `ctest --test-dir build`.
- Tests link `tetris_core` only and must never need a terminal.
  This is the payoff of the architecture rule, so protect it.
- Exercise the engine through its public API.
  If something is hard to test, that is a signal the API is wrong, not a reason to reach into internals.
- Shared helpers live in `tests/support.hpp`; file-local helpers go in an anonymous namespace at the top of the suite.
- `TEST_CASE` names are lowercase sentences describing the behaviour ("a second hold on the same piece is rejected"), not function names.
- Seed the RNG explicitly so a game is reproducible; that reproducibility is what makes regression tests possible.
- Test pure logic with real edge cases: rotations, kicks against each wall and in a well, multi-line and non-adjacent clears, the level cap, the hold lockout and when it clears, save/load round trips including the upcoming piece sequence.
- Do not unit test the ncurses layer.
  Verify the UI by running `./build/tetris` and looking at it.

## Verification

For UI or gameplay-feel changes, run the real binary rather than reasoning from the code.
`-DTETRIS_BUILD_UI=OFF` builds the engine and tests without ncurses when only the core changed.

## Code Quality

- No commented-out code unless specified
- No unused imports or variables
- Keep functions under 50 lines when possible

## Comments

Write code that explains itself; comment only what the code cannot say.
Over-commenting is a common AI tell, so resist it.

- Comment the **why**, not the **what**. Delete any comment that restates the code.
- No banner/header blocks, section dividers, or step-by-step narration of obvious
  code. A file does not need a comment announcing each region.
- A comment earns its place only when it captures something the code can't: a
  non-obvious decision, a gotcha or workaround, why a value is what it is, or a
  link to a spec or issue.
- The existing comments are the model: they record traps (clearing the hold flag
  in a shared helper silently restores unlimited swapping) and rationale (an enum
  return leaves room for `Locked` without the engine learning about sound).
- Prefer self-documenting names and small functions over explanatory comments.
- Keep doc comments minimal: a one-line purpose on an exported type or function is
  plenty; don't write JSDoc that just repeats the signature.
- When in doubt, leave the comment out.

## Writing

- No em dashes (U+2014) in generated content: docs, comments, commit messages,
  READMEs, specs. They read as AI-generated.
- Use a hyphen for `term - description` separators; rephrase prose with commas,
  parentheses, or a colon. Avoid en dashes and the ellipsis character too.
