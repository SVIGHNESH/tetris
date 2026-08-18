# Feature: Drop scoring

**From build-plan:** feature 14
**Status:** complete

## Goal

Reward hurrying: guideline drop points on top of the clear-points table.

## What was built

- `kSoftDropPoints = 1` and `kHardDropPoints = 2` in `tables.hpp`, flat per row, never scaled by level.
- `soft_drop()` scores one point per successful row; a failed drop scores nothing.
- `hard_drop()` scores two points per row between the piece and its landing position.
- No save format change: points already travel through the existing `points` field.

## Tests

- `apply()` in `tests/support.hpp` now returns the rows the hard drop fell, and `play_greedy` passes it to the callback, so the long played-game case still verifies every score delta exactly.
- Scoring cases updated to include drop points; new case for soft-drop points including the no-op at the floor.
- 82/82 tests pass; build clean under `-Werror`.
