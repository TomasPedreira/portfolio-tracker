#include "portfolio_app.h"

#include "raylib.h"
#include "sample_data.h"

#include <algorithm>

PortfolioApp::PortfolioApp()
    : portfolio_(SampleData::make_portfolio()), renderer_(), scroll_offset_(0.0f) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Portfolio Tracker");
    SetWindowMinSize(1100, 620);
    SetTargetFPS(60);
}

PortfolioApp::~PortfolioApp() {
    CloseWindow();
}

bool PortfolioApp::should_close() const {
    return WindowShouldClose();
}

void PortfolioApp::input() {
    scroll_offset_ -= GetMouseWheelMove() * 34.0f;

    if (IsKeyDown(KEY_DOWN)) {
        scroll_offset_ += 8.0f;
    }

    if (IsKeyDown(KEY_UP)) {
        scroll_offset_ -= 8.0f;
    }

    if (IsKeyPressed(KEY_HOME)) {
        scroll_offset_ = 0.0f;
    }
}

void PortfolioApp::update() {
    const float max_scroll = renderer_.max_scroll_offset(portfolio_, GetScreenHeight());
    scroll_offset_ = std::clamp(scroll_offset_, 0.0f, max_scroll);
}

void PortfolioApp::render() const {
    BeginDrawing();
    ClearBackground(Color{12, 30, 48, 255});
    renderer_.draw(portfolio_, scroll_offset_);
    EndDrawing();
}
