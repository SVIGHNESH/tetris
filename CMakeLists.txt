cmake_minimum_required(VERSION 3.20)
project(tetris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

option(TETRIS_BUILD_TESTS "Build the unit tests" ON)
option(TETRIS_BUILD_UI "Build the ncurses front end" ON)

# Shared warning configuration. Every target in this project links this.
add_library(tetris_warnings INTERFACE)
target_compile_options(tetris_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow)

# ---------------------------------------------------------------------------
# tetris_core: the engine. No UI dependency of any kind.
# ---------------------------------------------------------------------------
add_library(tetris_core STATIC
  src/core/piece.cpp
  src/core/board.cpp
)
target_include_directories(tetris_core PUBLIC include)
target_link_libraries(tetris_core PRIVATE tetris_warnings)

# ---------------------------------------------------------------------------
# tetris: the executable. Core plus ncurses.
# ---------------------------------------------------------------------------
if(TETRIS_BUILD_UI)
  set(CURSES_NEED_NCURSES TRUE)
  find_package(Curses REQUIRED)

  add_executable(tetris
    src/ui/main.cpp
    src/ui/terminal.cpp
    src/ui/renderer.cpp
    src/ui/boss_mode.cpp
  )
  target_include_directories(tetris PRIVATE src/ui ${CURSES_INCLUDE_DIRS})
  target_link_libraries(tetris PRIVATE tetris_core tetris_warnings ${CURSES_LIBRARIES})
endif()

# ---------------------------------------------------------------------------
# tetris_tests: core only, no terminal required.
# ---------------------------------------------------------------------------
if(TETRIS_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(tetris_tests
    tests/test_tables.cpp
    tests/test_piece.cpp
    tests/test_board.cpp
  )
  target_link_libraries(tetris_tests PRIVATE tetris_core tetris_warnings Catch2::Catch2WithMain)

  enable_testing()
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(tetris_tests)
endif()
