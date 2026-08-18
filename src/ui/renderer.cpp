#include "renderer.hpp"

#include <ncurses.h>

#include <algorithm>
#include <array>
#include <string>

#include "tetris/tables.hpp"

namespace tetris::ui {
namespace {

// Cells are two characters wide so that a block reads as a square rather than
// a tall thin sliver.
constexpr int kCellWidth = 2;
constexpr int kPanelWidth = 12;  // including both borders
constexpr int kPanelGap = 2;
constexpr int kPreviewHeight = 6;  // including both borders
constexpr int kStatsHeight = 6;
constexpr int kHintHeight = 1;

// Menus are padded to a fixed content width so that adding a status line does
// not make the box jump to a different size under the player's eyes.
constexpr int kMenuContentWidth = 30;

int attr(chtype attribute) { return static_cast<int>(attribute); }

void draw_box(int top, int left, int height, int width,
              std::string_view label = {}, bool dim = false) {
  if (dim) attron(attr(A_DIM));
  mvaddch(top, left, ACS_ULCORNER);
  mvaddch(top, left + width - 1, ACS_URCORNER);
  mvaddch(top + height - 1, left, ACS_LLCORNER);
  mvaddch(top + height - 1, left + width - 1, ACS_LRCORNER);
  for (int x = 1; x < width - 1; ++x) {
    mvaddch(top, left + x, ACS_HLINE);
    mvaddch(top + height - 1, left + x, ACS_HLINE);
  }
  for (int y = 1; y < height - 1; ++y) {
    mvaddch(top + y, left, ACS_VLINE);
    mvaddch(top + y, left + width - 1, ACS_VLINE);
  }

  if (!label.empty() && static_cast<int>(label.size()) + 4 <= width) {
    if (!dim) attron(attr(A_BOLD));
    mvaddch(top, left + 2, ' ');
    mvaddnstr(top, left + 3, label.data(), static_cast<int>(label.size()));
    mvaddch(top, left + 3 + static_cast<int>(label.size()), ' ');
    if (!dim) attroff(attr(A_BOLD));
  }
  if (dim) attroff(attr(A_DIM));
}

// Trims the empty rows and columns out of a shape so that a preview is
// centred on the piece rather than on its 4x4 origin box.
struct Bounds {
  int min_row = 0;
  int max_row = 0;
  int min_col = 0;
  int max_col = 0;
};

Bounds bounds_of(const Piece& piece) {
  const auto cells = piece.cells();
  Bounds bounds{cells[0].row, cells[0].row, cells[0].col, cells[0].col};
  for (const Offset& c : cells) {
    bounds.min_row = std::min(bounds.min_row, c.row);
    bounds.max_row = std::max(bounds.max_row, c.row);
    bounds.min_col = std::min(bounds.min_col, c.col);
    bounds.max_col = std::max(bounds.max_col, c.col);
  }
  return bounds;
}

// Centres text in a span, clipping rather than wrapping. A line that wraps
// pushes everything below it down a row and wrecks the layout, so anything
// too long to fit is cut instead.
void draw_centred(int row, int left, int width, std::string_view text) {
  const int screen_cols = getmaxx(stdscr);
  const int length = std::min(static_cast<int>(text.size()), screen_cols);
  int x = left + std::max(0, (width - length) / 2);
  x = std::clamp(x, 0, screen_cols - length);
  mvaddnstr(row, x, text.data(), length);
}

}  // namespace

Renderer::Layout Renderer::layout(const Game& game) const {
  Layout layout;
  layout.board_height = game.rows() + 2;
  layout.board_width = game.cols() * kCellWidth + 2;

  const int panel_height =
      2 * kPreviewHeight + kStatsHeight;
  const int height = std::max(layout.board_height, panel_height) + kHintHeight;
  const int width = layout.board_width + kPanelGap + kPanelWidth;

  layout.top = std::max(0, (terminal_.rows() - height) / 2);
  layout.left = std::max(0, (terminal_.cols() - width) / 2);
  layout.panel_left = layout.left + layout.board_width + kPanelGap;
  return layout;
}

bool Renderer::fits(const Game& game) const {
  const Layout l = layout(game);
  const int height = std::max(l.board_height, 2 * kPreviewHeight + kStatsHeight);
  const int width = l.board_width + kPanelGap + kPanelWidth;
  return terminal_.rows() >= height && terminal_.cols() >= width;
}

void Renderer::draw_cell(int row, int col, Cell cell, bool dim) {
  if (is_empty(cell)) {
    attron(attr(A_DIM));
    mvaddstr(row, col, ". ");
    attroff(attr(A_DIM));
    return;
  }

  if (terminal_.colour()) {
    const int pair = attr(COLOR_PAIR(colour_pair_for(cell)));
    attron(dim ? pair | attr(A_DIM) : pair);
    mvaddstr(row, col, "  ");
    attroff(dim ? pair | attr(A_DIM) : pair);
  } else {
    // Without colour the type still has to be readable, so the letter is the
    // block.
    const char letter = to_char(cell);
    const std::array<char, 3> block = {letter, letter, '\0'};
    if (dim) attron(attr(A_DIM));
    mvaddstr(row, col, block.data());
    if (dim) attroff(attr(A_DIM));
  }
}

void Renderer::draw_board(const Layout& layout, const Game& game) {
  draw_box(layout.top, layout.left, layout.board_height, layout.board_width);
  for (int row = 0; row < game.rows(); ++row) {
    for (int col = 0; col < game.cols(); ++col) {
      draw_cell(layout.top + 1 + row,
                layout.left + 1 + col * kCellWidth,
                game.at(row, col), false);
    }
  }
}

void Renderer::draw_preview(int top, int left, std::string_view label,
                            const Piece* piece, bool dim) {
  draw_box(top, left, kPreviewHeight, kPanelWidth, label, dim);

  // Clear the interior first: a preview that keeps a ghost of the previous
  // piece is worse than no preview at all.
  for (int y = 1; y < kPreviewHeight - 1; ++y) {
    for (int x = 1; x < kPanelWidth - 1; ++x) mvaddch(top + y, left + x, ' ');
  }
  if (piece == nullptr) return;

  const Bounds bounds = bounds_of(*piece);
  const int shape_rows = bounds.max_row - bounds.min_row + 1;
  const int shape_cols = bounds.max_col - bounds.min_col + 1;
  const int inner_rows = kPreviewHeight - 2;
  const int inner_cols = kPanelWidth - 2;

  const int offset_row = top + 1 + (inner_rows - shape_rows) / 2;
  const int offset_col =
      left + 1 + (inner_cols - shape_cols * kCellWidth) / 2;

  for (const Offset& c : piece->cells()) {
    draw_cell(offset_row + c.row - bounds.min_row,
              offset_col + (c.col - bounds.min_col) * kCellWidth,
              piece->cell(), dim);
  }
}

void Renderer::draw_panel(const Layout& layout, const Game& game) {
  const int left = layout.panel_left;
  int top = layout.top;

  draw_preview(top, left, "NEXT", &game.next(), false);
  top += kPreviewHeight;

  // The whole hold panel, border and label included, greys out while the
  // lockout is in force, so a player who presses hold twice can see why
  // nothing happened.
  const Piece* held = game.held() ? &*game.held() : nullptr;
  draw_preview(top, left, "HOLD", held, !game.can_hold());
  top += kPreviewHeight;

  const std::array<std::pair<const char*, int>, 3> stats = {{
      {"SCORE", game.score()},
      {"LEVEL", game.level()},
      {"LINES", game.lines_cleared()},
  }};

  for (const auto& [name, value] : stats) {
    mvprintw(top, left + 1, "%-5s", name);
    attron(attr(A_BOLD));
    mvprintw(top + 1, left + 1, "%-9d", value);
    attroff(attr(A_BOLD));
    top += 2;
  }
}

void Renderer::draw(const Game& game) {
  erase();
  const Layout l = layout(game);
  draw_board(l, game);
  draw_panel(l, game);

  // The hint is the one part of the screen that is allowed to go missing, so
  // it is only drawn when there is a spare row for it.
  const int hint_row =
      l.top + std::max(l.board_height, 2 * kPreviewHeight + kStatsHeight);
  if (hint_row < terminal_.rows()) {
    draw_centred(hint_row, 0, terminal_.cols(),
                 "arrows move  up rotate  down soft  space drop  c hold  "
                 "p pause");
  }
  refresh();
}

void Renderer::draw_too_small(const Game& game) {
  erase();
  const int needed_rows = game.rows() + 2;
  const int needed_cols = game.cols() * kCellWidth + 2 + kPanelGap + kPanelWidth;
  mvprintw(0, 0, "This terminal is %dx%d.", terminal_.cols(), terminal_.rows());
  mvprintw(1, 0, "Tetris needs at least %dx%d.", needed_cols, needed_rows);
  // Kept short enough not to wrap on the narrow terminals that get here.
  mvprintw(3, 0, "Resize, or press q to quit.");
  refresh();
}

void Renderer::draw_menu(std::string_view title,
                         const std::vector<std::string>& lines) {
  // The menu owns the screen rather than floating over the board. An overlay
  // box lands on top of the panel and leaves half words like "CORE" and
  // "EVEL" poking out from behind it, which reads as a corrupted screen.
  erase();

  int content = std::max(kMenuContentWidth, static_cast<int>(title.size()));
  for (const std::string& line : lines) {
    content = std::max(content, static_cast<int>(line.size()));
  }
  const int width = content + 6;
  const int height = static_cast<int>(lines.size()) + 4;

  const int top = std::max(0, (terminal_.rows() - height) / 2);
  const int left = std::max(0, (terminal_.cols() - width) / 2);

  draw_box(top, left, height, width, title);

  // Entries share a left edge. Centring each line individually leaves the
  // keys in a ragged column, which is harder to read than a plain list.
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& line = lines[i];
    mvaddnstr(top + 2 + static_cast<int>(i), left + 3, line.data(),
              std::min(static_cast<int>(line.size()), content));
  }
  refresh();
}

void Renderer::draw_game_over(const Game& game) {
  draw_menu("GAME OVER", {"score   " + std::to_string(game.score()),
                          "level   " + std::to_string(game.level()),
                          "lines   " + std::to_string(game.lines_cleared()),
                          "",
                          "r  play again",
                          "q  quit"});
}

}  // namespace tetris::ui
