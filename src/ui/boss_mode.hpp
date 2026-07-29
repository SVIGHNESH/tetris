#pragma once

#include "terminal.hpp"

namespace tetris::ui {

// Fills the screen with something that looks like work and blocks until a key
// is pressed. The engine never learns this exists.
void show_boss_mode(const Terminal& terminal);

}  // namespace tetris::ui
