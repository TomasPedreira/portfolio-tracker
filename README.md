# Portfolio Tracker

## Description
Portfolio Tracker is a C++ desktop app for tracking investment positions. It stores stocks, positions, and portfolio totals, then displays the data in a raylib graphical interface.

The current version uses sample data only. Manual input and persistent storage will be added later.

## Current Features
- Stores stocks with symbol, name, and current price
- Stores positions with stock, buy price, acquisition date, and amount
- Groups positions inside a portfolio
- Calculates invested value, current value, gain/loss, and gain/loss percentage
- Displays the portfolio in a raylib window
- Uses a simple game loop with input, update, and render stages

## Build
Raylib is required. CMake will try to find an installed raylib first, then fetch raylib 5.5 if needed.

On WSL/Linux, install the raylib desktop build dependencies first:

```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

Then build:

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
- `src/sample_data.*`: temporary sample positions

