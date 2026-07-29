#include "boss_mode.hpp"

#include <ncurses.h>

#include <algorithm>
#include <array>

namespace tetris::ui {
namespace {

constexpr std::array<const char*, 22> kBuildLog = {
    "$ cmake --build build -j8",
    "[  4%] Building CXX object CMakeFiles/core.dir/src/core/board.cpp.o",
    "[  9%] Building CXX object CMakeFiles/core.dir/src/core/piece.cpp.o",
    "[ 13%] Building CXX object CMakeFiles/core.dir/src/core/game.cpp.o",
    "[ 18%] Building CXX object CMakeFiles/core.dir/src/core/serialize.cpp.o",
    "[ 22%] Linking CXX static library libcore.a",
    "[ 27%] Building CXX object CMakeFiles/service.dir/src/router.cpp.o",
    "[ 31%] Building CXX object CMakeFiles/service.dir/src/handlers.cpp.o",
    "[ 36%] Building CXX object CMakeFiles/service.dir/src/metrics.cpp.o",
    "[ 40%] Linking CXX executable service",
    "[ 45%] Running unit tests",
    "     Start  1: board.clear_full_lines .......................   Passed",
    "     Start  2: board.fits ...................................   Passed",
    "     Start  3: piece.rotation_identity ......................   Passed",
    "     Start  4: game.gravity_interval ........................   Passed",
    "     Start  5: game.wall_kicks ..............................   Passed",
    "     Start  6: game.hold_lockout ............................   Passed",
    "     Start  7: serialize.round_trip .........................   Passed",
    "",
    "100% tests passed, 0 tests failed out of 66",
    "",
    "Total test time (real) =   1.84 sec",
};

}  // namespace

void show_boss_mode(const Terminal& terminal) {
  erase();
  const int visible = std::min(static_cast<int>(kBuildLog.size()),
                               std::max(0, terminal.rows() - 2));
  for (int i = 0; i < visible; ++i) {
    mvaddstr(i, 0, kBuildLog[static_cast<std::size_t>(i)]);
  }

  // A prompt with a visible cursor sitting in it is what sells the illusion.
  mvaddstr(visible, 0, "$ ");
  curs_set(1);
  move(visible, 2);
  refresh();

  terminal.wait_for_key();

  curs_set(0);
  erase();
  refresh();
}

}  // namespace tetris::ui
