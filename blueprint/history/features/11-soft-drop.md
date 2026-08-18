# Feature: Soft drop

**From build-plan:** feature 11
**Status:** built, awaiting review

## Goal

Pressing down should make the piece fall faster, not end its life.
Today `KEY_DOWN`, `j`, and space all map to `Move::Drop`, which teleports the piece to its landing position and locks it immediately.
That leaves no way to hurry a piece along while still being able to steer it, which is the single most-used input in modern Tetris.

After this, down and `j` move the piece one row per tick and reset the gravity counter, so holding the key accelerates the fall.
Space keeps the instant hard drop.

## In scope

- A new `Move::SoftDrop` value and a `Game::soft_drop()` that moves the falling piece down one row when it fits.
- Resetting the gravity counter on a successful soft drop, so a soft-dropped row does not also get a free gravity row in the same tick.
- Remapping `KEY_DOWN` and `j` to `Move::SoftDrop` in the UI; space stays `Move::Drop`.
- Updating the controls table in `README.md`.
- Engine tests for the new behaviour.

## Out of scope

- **Soft drop scoring.** No points per row dropped. The points table stays the only source of score, so `test_scoring.cpp` is untouched.
- **Lock delay.** A piece that cannot move down still locks on the normal gravity schedule; soft drop never locks it early.
- **Key auto-repeat tuning (DAS/ARR).** Repeat rate stays whatever the terminal gives us.
- **Save format.** `Move` is transient input and is never serialised, so the format version stays at 1.
- **Hard drop removal.** Space keeps it, and `hard_drop()` is unchanged.

## Build loop

Build one step at a time, never the whole feature at once.

1. Plan mode lays out the step before any code.
2. The AI implements just that step.
3. It shows the diff (not full files); you read it and understand it.
4. You approve, then choose whether to commit a checkpoint or roll straight on.
   Checkpoints are optional; `/complete` makes the real feature-level commit at the end.

Never accept a step you haven't read. If a diff is too big to review, the step was too big, so split it.

## Build steps

- [x] **Step 1 - engine: `Move::SoftDrop`** - add the enum value to `move.hpp`, a private `void soft_drop()` to `Game`, and its `apply_move` case. `soft_drop()` translates the falling piece down one row if `board_.fits()`, then calls `reset_gravity()`; when it does not fit it does nothing and returns, leaving `gravity_tick()` to lock the piece on its normal schedule. Returns `false` from `apply_move` (it never locks), so `tick()` still runs gravity afterwards. *Done when:* `cmake --build build` is clean under `-Werror` (the `switch` in `apply_move` is exhaustive) and `ctest --test-dir build` still shows 67/67.

- [x] **Step 2 - engine tests** - add `tests/test_softdrop.cpp` to the `tetris_tests` target in `CMakeLists.txt`, covering: a soft drop moves the piece exactly one row; it does not lock the piece (the same piece is still falling and the board has no new settled cells); a soft drop against the floor or a stack is a no-op that leaves the piece falling; repeated soft drops reach the same row as `landing_position()` without ever locking early; and the piece still locks by gravity after arriving. *Done when:* the new cases pass and the suite total rises from 67.

- [x] **Step 3 - UI keymap and docs** - in `move_for_key()`, split `KEY_DOWN` and `j` off to `Move::SoftDrop` and leave `' '` on `Move::Drop`. Update the controls table in `README.md` so down/`j` read "soft drop (faster fall)" and space reads "hard drop". *Done when:* running `./build/tetris`, holding down visibly accelerates the fall while the piece stays steerable left/right, and space still slams it into place.

## Files / areas

- `include/tetris/move.hpp` - the new enum value and a comment on how it differs from `Drop`.
- `include/tetris/game.hpp` - `void soft_drop();` next to `hard_drop()` in the private section.
- `src/core/game.cpp` - `soft_drop()` and the `apply_move` case.
- `tests/test_softdrop.cpp` - new suite.
- `CMakeLists.txt` - register the new test file.
- `src/ui/main.cpp` - `move_for_key()` only.
- `README.md` - controls table.

## Data / contracts

- **`enum class Move`** is load-bearing: `apply_move()` switches on it exhaustively under `-Werror`, so adding a value is a compile-time contract change. `main.cpp` is the only other file that names `Move` values.
- **The save format does not change.** `Move` is per-tick input and never serialised; `kFormatVersion` stays 1 and existing `tetris.save` files keep loading.
- `Move::None` remains the "no input this tick" value; soft drop must not be conflated with it.

## Testing

The test gate is **on** (`ctest --test-dir build` is declared in `AGENTS.md`), and this feature is engine logic, so Step 2 ships tests in the same diff.

- **In-scope for tests:** `soft_drop()` behaviour - the one-row move, the no-op at the floor, the absence of an early lock, and the interaction with the gravity counter. These are pure engine logic reachable through `Game::tick(Move::SoftDrop)`.
- **Not unit tested:** the `move_for_key()` mapping and the felt acceleration. Those live in the ncurses layer, which links no test target. Verify Step 3 by running `./build/tetris`.
- Use a fixed seed so the piece sequence is reproducible, per the existing suites.

## Notes for the AI

- **The engine must not learn that a keyboard exists.** `soft_drop()` takes no input and knows nothing about key repeat; the UI decides how often to send the move.
- **Reset gravity on a successful soft drop, not on a failed one.** Without the reset, a soft drop in the same tick as a gravity step moves the piece two rows. Resetting after a *failed* soft drop would let a player hover a piece on the floor indefinitely, which is lock delay - explicitly out of scope.
- `try_move()` currently returns `void`, so it cannot report whether the piece moved. Change it to return `bool` (a one-line change, and `soft_drop()` is the only caller that needs the answer) rather than duplicating the fits-test inside `soft_drop()`.
- Follow the existing style: `snake_case` private members on `Game`, comments that record the trap rather than restate the code.
- Test names are lowercase sentences describing behaviour, matching the existing suites.
