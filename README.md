# Portfolio Tracker

## Description
Portfolio Tracker is a C++ desktop app for tracking investment positions. It stores stocks, positions, and portfolio totals, then displays the data in a raylib graphical interface.

## Current Features
- Stores stocks with symbol, name, and current price
- Stores positions with stock, buy price, acquisition date, and amount
- Groups positions inside a portfolio
- Calculates invested value, current value, gain/loss, and gain/loss percentage
- Displays the portfolio in a raylib window
- Loads/saves portfolio data from a TSV file (`--file <path>`)
- Uses a simple game loop with input, update, and render stages

## Build
Raylib is required. CMake will try to find an installed raylib first, then fetch raylib 5.5 if needed.

On Linux or WSL, raylib may need desktop build dependencies:

```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

On Windows, use your normal CMake toolchain. If raylib is not installed, CMake will fetch the raylib release archive during configuration.

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/portfolio_tracker
```

## Project Structure
- `src/main.cpp`: application entry point and main loop
- `src/portfolio_app.*`: window lifecycle and app orchestration
- `src/portfolio_renderer.*`: raylib drawing code
- `src/portfolio.*`: portfolio collection and totals
- `src/position.*`: position model and calculations
- `src/stock.*`: stock model

