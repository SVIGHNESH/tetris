# Feature: Ghost piece

**From build-plan:** feature 12
**Status:** complete

## Goal

Show where the falling piece will land, so hard drops stop being a leap of faith.

## What was built

- `Renderer::draw_ghost()` draws the cells of `Game::landing_position()` as a dim `::` marker in the piece's colour (plain dim in monochrome), skipping any cell already occupied on screen so the ghost never paints over the falling piece or settled blocks.
- Called from `draw_board()`; skipped once the game is over.
- No engine change: `landing_position()` already existed and was already tested (`test_game.cpp` proves it does not disturb the game).

## Verification

- Build clean under `-Werror`; 74/74 tests pass (no engine logic changed, so no new tests per the test gate).
- Ran `./build/tetris` in a pty and confirmed the `::` ghost renders at the landing rows below the falling piece.
