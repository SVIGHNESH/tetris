#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "terminal.hpp"
#include "tetris/game.hpp"

namespace tetris::ui {

// Draws the game. Reads only the public Game API, so the renderer cannot
// disturb the engine no matter what it does.
class Renderer {
 public:
  explicit Renderer(const Terminal& terminal) : terminal_(terminal) {}

  // False when the window cannot hold the board and the panel. The caller
  // shows the "make me bigger" screen instead of drawing a broken board.
  bool fits(const Game& game) const;

  void draw(const Game& game);
  void draw_too_small(const Game& game);

  // Overlays, drawn on top of whatever is already on screen.
  void draw_menu(std::string_view title, const std::vector<std::string>& lines);
  void draw_game_over(const Game& game);

 private:
  struct Layout {
    int top = 0;
    int left = 0;
    int board_height = 0;
    int board_width = 0;
    int panel_left = 0;
  };

  Layout layout(const Game& game) const;

  void draw_board(const Layout& layout, const Game& game);
  void draw_panel(const Layout& layout, const Game& game);
  void draw_preview(int top, int left, std::string_view label,
                    const Piece* piece, bool dim);
  void draw_cell(int row, int col, Cell cell, bool dim);

  const Terminal& terminal_;
};

}  // namespace tetris::ui
