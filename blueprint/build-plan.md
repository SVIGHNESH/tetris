# Build Plan

The features that make up this project, in build order.
Shipped items are checked; they were built before the Blueprint was adopted, so they have no archived specs under `blueprint/history/`.
Ordering follows the build order in [PLAN.md](../PLAN.md) section 13.

Run `/feature` with no number to spec the next unchecked item, or `/feature "name"` to propose a genuinely new one.

## Shipped

- [x] 1. **Scaffold** - CMake with C++20, the three targets, warnings as errors, Catch2 via `FetchContent`
- [x] 2. **Types and tables** - `Cell`, `Tetromino`, `Offset`, `Move`, plus the constexpr tetromino and gravity tables
- [x] 3. **Piece** - value-semantic `cells()`, `rotated()`, `translated()`
- [x] 4. **Board** - `at`, `in_bounds`, pure `fits`, `lock`, `clear_full_lines`
- [x] 5. **Game skeleton** - spawn, gravity, lateral movement, lock, respawn, game over, debug `print()`
- [x] 6. **Rotation with wall kicks** - table-driven candidate offsets, first fit wins
- [x] 7. **Scoring and levels** - points table scaled by level, 10 lines per level, cap at 19
- [x] 8. **Hold and next** - 7-bag draw, next preview, hold with the once-per-piece lockout
- [x] 9. **ncurses UI** - RAII `Terminal`, `Renderer`, fixed-timestep main loop, colour per tetromino
- [x] 10. **Extras** - pause menu, boss mode, versioned save and load, warning cleanup, full test coverage

## Next

- [x] 11. **Soft drop** - the down key accelerates the fall by one row per tick instead of hard dropping; space keeps the instant drop
- [x] 12. **Ghost piece** - the ncurses renderer draws the falling piece's landing position as a dim outline, using the existing `landing_position()` accessor
- [x] 13. **Lock delay** - a short grace window after touchdown, reset by movement with a cap; serialised, bumping the save format to version 2
- [x] 14. **Drop scoring** - 1 point per soft-dropped row, 2 per hard-dropped row, on top of the existing clear-points table

Describe a feature in chat or run `/feature "<name>"` and it will propose the build-plan line for review before anything is spec'd.
Continue numbering from 12; do not renumber the shipped items.
