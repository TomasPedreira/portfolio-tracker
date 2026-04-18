# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
cmake -S . -B build
cmake --build build
./build/portfolio_tracker                  # start fresh
./build/portfolio_tracker --file path.tsv  # load existing portfolio
```

Raylib 5.5 is fetched automatically by CMake if not present locally. On Linux/WSL, install desktop libs first: `libx11-dev libxrandr-dev` etc.

There are no lint or test commands — this is a C++20 desktop GUI app with no test infrastructure.

## Architecture

Single-window desktop app built with **Raylib** (graphics) and **CMake**. All state lives in memory; persistence is a local TSV file.

**Core classes:**

- `Portfolio` — owns the stock registry and position list; computes totals (cost basis, current value, gain/loss). Stocks are shared across positions by symbol, so updating a price propagates everywhere.
- `PortfolioApp` — main loop: polls input → updates state → delegates rendering. Owns UI state (which panels are open, text field contents).
- `PortfolioRenderer` — all Raylib draw calls; stateless aside from font/color constants.
- `PortfolioFile` — serializes/deserializes the TSV format (symbol, name, price, buy price, amount, date).
- `AppConfig` — reads/writes `portfolio_tracker.cfg` to remember the last-opened file path.

**Data flow:** User fills the add-position panel → `PortfolioApp` validates and calls `Portfolio::addPosition` → `Portfolio` updates the stock registry and recalculates totals → `PortfolioRenderer` draws the new state → on save, `PortfolioFile` writes TSV.

## Critical Implementation Details

### Column Positioning (Stock vs Position Rows)

Stock rows and position rows have **different column layouts**. Both renderer and app must compute columns identically:

- **Stock rows** (`stock_cols()` in renderer, `make_cols()` in app): Symbol, Name, Current, Shares, Value, Gain
- **Position rows** (`pos_cols()` in renderer, `make_pos_cols()` in app): Date, Shares, Buy, Value, Gain

When a stock is expanded, a position sub-header row (`kPanelColHdrH = 34px`) is drawn before position rows. Click detection must account for this by skipping the header height when checking position clicks.

**Important:** If you change column positions in the renderer, update the corresponding struct in `portfolio_app.cpp` — click detection depends on exact x/y alignment.

### Layout Constants

Both files define layout constants that must stay synchronized:
- `src/portfolio_renderer.cpp` — `kMargin`, `kHeaderHeight`, `kSummaryHeight`, etc. (display values)
- `src/portfolio_app.cpp` — `kMargin`, `kHdrH`, `kSumH`, etc. (must match renderer)

### GUI Utilities

- `gui::draw_text_bold()` — draws text with a darkened drop shadow for emphasis. Used for column headers and form labels.
