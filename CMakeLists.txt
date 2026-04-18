cmake_minimum_required(VERSION 3.20)
project(portfolio_tracker LANGUAGES CXX)

include(FetchContent)

find_package(raylib QUIET)

if(NOT raylib_FOUND)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(portfolio_tracker
    src/main.cpp
    src/app_config.cpp
    src/gui/utils.cpp
    src/portfolio_app.cpp
    src/portfolio_file.cpp
    src/portfolio.cpp
    src/portfolio_renderer.cpp
    src/position.cpp
    src/stock.cpp
)

set_target_properties(portfolio_tracker PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

target_link_libraries(portfolio_tracker PRIVATE raylib)
