#include "terminal.hpp"

#include <ncurses.h>

namespace tetris::ui {
namespace {

struct ColourChoice {
  Cell cell;
  short colour;
};

// There is no orange in the eight colour palette, so L borrows white. The
// rest follow the conventional tetromino colours closely enough to be
// recognisable.
constexpr ColourChoice kColours[] = {
    {Cell::I, COLOR_CYAN},   {Cell::J, COLOR_BLUE},
    {Cell::L, COLOR_WHITE},  {Cell::O, COLOR_YELLOW},
    {Cell::S, COLOR_GREEN},  {Cell::T, COLOR_MAGENTA},
    {Cell::Z, COLOR_RED},
};

}  // namespace

short colour_pair_for(Cell cell) {
  return static_cast<short>(cell);  // Cell::Empty is 0, which is no pair
}

Terminal::Terminal() {
  initscr();
  cbreak();     // deliver keys immediately, but leave the interrupt key alone
  noecho();     // the game draws the board, not what the player typed
  keypad(stdscr, TRUE);  // arrow keys arrive as single KEY_ values
  curs_set(0);
  nodelay(stdscr, TRUE);

  colour_ = has_colors();
  if (colour_) {
    start_color();
    use_default_colors();
    for (const ColourChoice& choice : kColours) {
      init_pair(colour_pair_for(choice.cell), COLOR_BLACK, choice.colour);
    }
  }
}

Terminal::~Terminal() { endwin(); }

int Terminal::rows() const { return LINES; }
int Terminal::cols() const { return COLS; }

std::optional<int> Terminal::next_key() const {
  const int key = getch();
  if (key == ERR) return std::nullopt;
  return key;
}

int Terminal::wait_for_key() const {
  set_blocking(true);
  const int key = getch();
  set_blocking(false);
  return key;
}

void Terminal::set_blocking(bool blocking) const {
  nodelay(stdscr, blocking ? FALSE : TRUE);
}

}  // namespace tetris::ui
