#include <ncurses.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "boss_mode.hpp"
#include "renderer.hpp"
#include "terminal.hpp"
#include "tetris/game.hpp"

namespace {

using namespace tetris;
using namespace tetris::ui;
using Clock = std::chrono::steady_clock;

// The gravity table is expressed in ticks, so the tick rate is what turns it
// into wall clock speed: at sixty a second, level 0 drops a row every 830ms
// and level 19 every 67ms.
constexpr double kTicksPerSecond = 60.0;
constexpr double kSecondsPerTick = 1.0 / kTicksPerSecond;

// Long enough that the loop is not a spin, short enough that input still
// feels immediate.
constexpr auto kFrameNap = std::chrono::milliseconds(2);

constexpr const char* kSavePath = "tetris.save";

struct Options {
  int rows = Game::kDefaultRows;
  int cols = Game::kDefaultCols;
  std::optional<std::uint32_t> seed;
  bool help = false;
  std::optional<std::string> error;
};

bool parse_int(std::string_view text, int& out) {
  const auto* end = text.data() + text.size();
  const auto result = std::from_chars(text.data(), end, out);
  return result.ec == std::errc() && result.ptr == end;
}

Options parse_options(int argc, char** argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      options.help = true;
      return options;
    }

    const bool needs_value = i + 1 < argc;
    if (arg == "--rows" && needs_value) {
      if (!parse_int(argv[++i], options.rows) || options.rows < 6) {
        options.error = "--rows needs a number of at least 6";
      }
    } else if (arg == "--cols" && needs_value) {
      if (!parse_int(argv[++i], options.cols) || options.cols < 5) {
        options.error = "--cols needs a number of at least 5";
      }
    } else if (arg == "--seed" && needs_value) {
      int seed = 0;
      if (!parse_int(argv[++i], seed) || seed < 0) {
        options.error = "--seed needs a non-negative number";
      } else {
        options.seed = static_cast<std::uint32_t>(seed);
      }
    } else {
      options.error = "unrecognised argument: " + std::string(arg);
    }
    if (options.error) return options;
  }
  return options;
}

void print_usage() {
  std::cout << "usage: tetris [--rows N] [--cols N] [--seed N]\n\n"
               "  --rows N   board height (default "
            << Game::kDefaultRows
            << ")\n"
               "  --cols N   board width (default "
            << Game::kDefaultCols
            << ")\n"
               "  --seed N   piece sequence seed, for a reproducible game\n";
}

std::uint32_t random_seed() {
  std::random_device device;
  return device();
}

std::optional<Move> move_for_key(int key) {
  switch (key) {
    case KEY_LEFT:
    case 'a':
    case 'h':
      return Move::Left;
    case KEY_RIGHT:
    case 'd':
    case 'l':
      return Move::Right;
    case KEY_UP:
    case 'w':
    case 'k':
    case 'x':
      return Move::RotateCW;
    case 'z':
      return Move::RotateCCW;
    case KEY_DOWN:
    case 'j':
    case ' ':
      return Move::Drop;
    case 'c':
      return Move::Hold;
    default:
      return std::nullopt;
  }
}

bool save_game(const Game& game) {
  std::ofstream file(kSavePath, std::ios::trunc);
  if (!file) return false;
  game.save(file);
  return file.good();
}

std::optional<Game> load_game() {
  std::ifstream file(kSavePath);
  if (!file) return std::nullopt;
  return Game::load(file);
}

// What the loop should do after a menu closes.
enum class MenuResult { Resume, Restart, Quit };

MenuResult run_pause_menu(const Terminal& terminal, Renderer& renderer,
                          Game& game) {
  std::string status;
  while (true) {
    std::vector<std::string> lines = {
        "p  resume", "s  save", "o  load", "b  boss", "r  restart", "q  quit"};
    if (!status.empty()) {
      lines.emplace_back("");
      lines.push_back(status);
    }
    renderer.draw_menu("PAUSED", lines);

    switch (terminal.wait_for_key()) {
      case 'p':
      case 27:  // escape
        return MenuResult::Resume;
      case 'q':
        return MenuResult::Quit;
      case 'r':
        return MenuResult::Restart;
      case 'b':
        show_boss_mode(terminal);
        break;
      case 's':
        status = save_game(game) ? "saved to tetris.save"
                                 : "could not write tetris.save";
        break;
      case 'o': {
        std::optional<Game> loaded = load_game();
        if (loaded) {
          game = std::move(*loaded);
          return MenuResult::Resume;
        }
        status = "no readable save in tetris.save";
        break;
      }
      default:
        break;
    }
  }
}

int run(const Options& options) {
  Terminal terminal;
  Renderer renderer(terminal);

  const std::uint32_t seed = options.seed.value_or(random_seed());
  Game game(options.rows, options.cols, seed);

  std::deque<Move> pending;
  double accumulator = 0.0;
  auto previous = Clock::now();
  bool running = true;

  auto restart = [&] {
    game = Game(options.rows, options.cols,
                options.seed.value_or(random_seed()));
    pending.clear();
    accumulator = 0.0;
    previous = Clock::now();
  };

  while (running) {
    while (const std::optional<int> key = terminal.next_key()) {
      if (const std::optional<Move> move = move_for_key(*key)) {
        // Nothing drains the queue once the game is over, so nothing should
        // be added to it either.
        if (!game.game_over()) pending.push_back(*move);
        continue;
      }
      switch (*key) {
        case 'q':
          running = false;
          break;
        case 'p':
        case 27:
          switch (run_pause_menu(terminal, renderer, game)) {
            case MenuResult::Quit: running = false; break;
            case MenuResult::Restart: restart(); break;
            case MenuResult::Resume: break;
          }
          // The menu blocked, so the clock has run on without the game.
          previous = Clock::now();
          accumulator = 0.0;
          break;
        case 'b':
          show_boss_mode(terminal);
          previous = Clock::now();
          accumulator = 0.0;
          break;
        case 'r':
          // Only from the game over screen. A single unconfirmed keypress
          // that throws away a game in progress is a trap; mid game, restart
          // lives behind the pause menu.
          if (game.game_over()) restart();
          break;
        default:
          break;
      }
    }
    if (!running) break;

    const auto now = Clock::now();
    const double elapsed =
        std::chrono::duration<double>(now - previous).count();
    previous = now;

    // A window too small to draw honestly freezes the game rather than
    // dropping the player into a board they cannot see.
    if (!renderer.fits(game)) {
      renderer.draw_too_small(game);
      accumulator = 0.0;
      std::this_thread::sleep_for(kFrameNap);
      continue;
    }

    if (!game.game_over()) {
      accumulator += elapsed;
      // Catching up on more than a handful of ticks after a long stall would
      // teleport the piece downwards, so the backlog is capped.
      accumulator = std::min(accumulator, kSecondsPerTick * 8);
      while (accumulator >= kSecondsPerTick) {
        accumulator -= kSecondsPerTick;
        const Move move = pending.empty() ? Move::None : pending.front();
        if (!pending.empty()) pending.pop_front();
        game.tick(move);
      }
    }

    if (game.game_over()) {
      renderer.draw_game_over(game);
    } else {
      renderer.draw(game);
    }
    std::this_thread::sleep_for(kFrameNap);
  }

  return game.score();
}

}  // namespace

int main(int argc, char** argv) {
  const Options options = parse_options(argc, argv);
  if (options.error) {
    std::cerr << *options.error << "\n\n";
    print_usage();
    return 2;
  }
  if (options.help) {
    print_usage();
    return 0;
  }

  // The Terminal lives inside run(), so ncurses has already put the terminal
  // back the way it found it by the time this prints.
  const int score = run(options);
  std::cout << "final score: " << score << '\n';
  return 0;
}
