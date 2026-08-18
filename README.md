# Tetris

A C++20 reimplementation of [Stephen Brennan's Tetris](https://brennan.io/2015/06/12/tetris-reimplementation/).

The design idea carried over from the original is the strict separation between the engine and the terminal.
`tetris_core` knows nothing about ncurses and does not link it, which is what makes the whole engine testable without a terminal.
See [PLAN.md](PLAN.md) for the reasoning behind the structure and for the deliberate deviations from the original.

## Building

Needs a C++20 compiler, CMake 3.20 or newer, and ncurses.
Catch2 is fetched during configuration, so the first build needs network access.

```sh
cmake -S . -B build
cmake --build build
```

Configure with `-DTETRIS_BUILD_TESTS=OFF` to skip Catch2 entirely, or `-DTETRIS_BUILD_UI=OFF` to build the engine and its tests without ncurses.

## Playing

```sh
./build/tetris                       # a 20x10 board
./build/tetris --rows 24 --cols 12   # any board size
./build/tetris --seed 42             # a reproducible piece sequence
```

| Key | Action |
| --- | --- |
| left / right, `a` / `d`, `h` / `l` | move |
| up, `w`, `k`, `x` | rotate clockwise |
| `z` | rotate counter-clockwise |
| down, `j` | soft drop: one row per tick, hold to fall faster |
| space | hard drop: straight to the bottom and lock |
| `c` | hold |
| `p`, escape | pause menu: save, load, boss mode, restart, quit |
| `b` | boss mode |
| `q` | quit |

Hold is once per piece.
The hold panel greys out while the lockout is in force, and comes back when the piece settles.

Saving writes `tetris.save` in the working directory.
A save records the generator state and the current bag as well as the board, so the upcoming pieces after a load are identical rather than merely plausible.

## Tests

```sh
ctest --test-dir build
```

The engine is exercised entirely through its public API.
Two of the tests play real games to completion: level progression and the points table are checked by playing far enough to reach level 19, using copies of the `Game` itself as the lookahead for choosing placements.
