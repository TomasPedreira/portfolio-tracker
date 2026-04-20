#include "portfolio_app.h"

#include "app_config.h"
#include "portfolio_file.h"
#include "position.h"
#include "stock.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

// ── Layout constants (must match renderer) ─────────────────────────────────────
static constexpr int kMargin       = 32;
static constexpr int kHdrH         = 126;
static constexpr int kSumH         = 110;
static constexpr int kTblGap       = 28;
static constexpr int kPanelTitleH  = 38;
static constexpr int kPanelColHdrH = 34;
static constexpr int kStockRowH    = 38;
static constexpr int kPosRowH      = 32;
static constexpr int kAddStockFormH = 124;

// Add-stock form internal layout (matches renderer)
static constexpr int kAsFldH  = 30;
static constexpr int kAsLblFont = 13;
static constexpr int kAsPad   = 14;
static constexpr int kAsGap   = 12;

static int table_top_px() { return kHdrH + kSumH + kTblGap; }

// Column x positions — must mirror stock_cols() and pos_cols() in renderer.
struct PanelCols {
    int bx, bw;
    int arrow_x, sym_x, name_x, cur_x, amt_x, buy_x, val_x, gain_x, date_x, del_x;
};
struct PosCols {
    int dash_x, date_x, amt_x, buy_x, val_x, diff_x, gain_x, del_x;
};

static PanelCols make_cols(int bx, int bw) {
    PanelCols c{};
    c.bx     = bx;  c.bw     = bw;
    c.arrow_x = bx + 10;
    c.sym_x  = bx + 40;
    c.name_x = bx + bw * 16 / 100;
    c.cur_x  = bx + bw * 44 / 100;
    c.amt_x  = bx + bw * 58 / 100;
    c.buy_x  = bx + bw * 48 / 100;
    c.val_x  = bx + bw * 71 / 100;
    c.gain_x = bx + bw * 83 / 100;
    c.date_x = bx + bw * 14 / 100;
    c.del_x  = bx + bw - 34;
    return c;
}

static PosCols make_pos_cols(int bx, int bw) {
    PosCols c{};
    c.dash_x = bx + 10;
    c.date_x = bx + bw * 14 / 100;
    c.amt_x  = bx + bw * 34 / 100;
    c.buy_x  = bx + bw * 48 / 100;
    c.val_x  = bx + bw * 60 / 100;
    c.diff_x = bx + bw * 72 / 100;
    c.gain_x = bx + bw * 84 / 100;
    c.del_x  = bx + bw - 34;
    return c;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

PortfolioApp::PortfolioApp(Portfolio portfolio, std::string file_path)
    : portfolio_(std::move(portfolio)), renderer_(), scroll_offset_(0.f),
      panel_(), save_popup_(), panel_state_(),
      backspace_timer_(0.f), file_path_(std::move(file_path)) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1800, 900, "Portfolio Tracker");
    SetWindowMinSize(1600, 760);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
}

PortfolioApp::~PortfolioApp() { CloseWindow(); }

bool PortfolioApp::should_close() const { return WindowShouldClose(); }

// ── Input ─────────────────────────────────────────────────────────────────────

void PortfolioApp::input() {
    const int sh = GetScreenHeight();
    const int sw = GetScreenWidth();

    // ── 1. Save popup (full modal) ─────────────────────────────────────────────
    if (save_popup_.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { save_popup_.open = false; save_popup_.error.clear(); return; }
        if (IsKeyPressed(KEY_ENTER))  { try_save_as(); return; }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            constexpr int kPW = 480, kPH = 190, kBW = 100, kBH = 36;
            const int px = (sw - kPW) / 2, py = (sh - kPH) / 2;
            const int cx = px + kPW - 24 - kBW;
            const int canx = cx - kBW - 12;
            const int by = py + kPH - 24 - kBH;
            const Vector2 m = GetMousePosition();
            if (m.x >= cx   && m.x <= cx   + kBW && m.y >= by && m.y <= by + kBH) { try_save_as(); return; }
            if (m.x >= canx && m.x <= canx + kBW && m.y >= by && m.y <= by + kBH)
                { save_popup_.open = false; save_popup_.error.clear(); return; }
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            const int len = (int)strlen(save_popup_.filename);
            if (ch >= 32 && ch <= 125 && len < 511)
                { save_popup_.filename[len] = (char)ch; save_popup_.filename[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            const int len = (int)strlen(save_popup_.filename);
            if (len > 0) save_popup_.filename[len-1] = '\0';
        }
        return;
    }

    // ── 2. Portfolio panel keyboard capture ────────────────────────────────────
    if (panel_state_.editing_price) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            panel_state_.editing_price = false;
            memset(panel_state_.price_buf, 0, sizeof(panel_state_.price_buf));
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) { try_confirm_price(); return; }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            const int len = (int)strlen(panel_state_.price_buf);
            if (len > 0) panel_state_.price_buf[len-1] = '\0';
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            const int len = (int)strlen(panel_state_.price_buf);
            if ((ch >= '0' && ch <= '9' || ch == '.') && len < 63)
                { panel_state_.price_buf[len] = (char)ch; panel_state_.price_buf[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            handle_portfolio_panel_click(GetMousePosition(), sw, sh);
        return;
    }

    if (panel_state_.editing_position_idx >= 0) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            panel_state_.editing_position_idx = -1;
            panel_state_.editing_stock_idx    = -1;
            panel_state_.error.clear();
            memset(panel_state_.edit_buf, 0, sizeof(panel_state_.edit_buf));
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) { try_confirm_position_edit(); return; }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            const int len = (int)strlen(panel_state_.edit_buf);
            if (len > 0) panel_state_.edit_buf[len-1] = '\0';
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            const int len = (int)strlen(panel_state_.edit_buf);
            if (len < 127) {
                const bool numeric = panel_state_.editing_field != PositionEditField::Date;
                if ((numeric && (ch >= '0' && ch <= '9' || ch == '.')) ||
                    (!numeric && ch >= 32 && ch <= 125))
                    { panel_state_.edit_buf[len] = (char)ch; panel_state_.edit_buf[len+1] = '\0'; }
            }
            ch = GetCharPressed();
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            handle_portfolio_panel_click(GetMousePosition(), sw, sh);
        return;
    }

    if (panel_state_.add_open) {
        if (IsKeyPressed(KEY_ESCAPE)) { panel_state_.add_open = false; panel_state_.error.clear(); return; }
        if (IsKeyPressed(KEY_ENTER))  { try_add_stock(); return; }
        if (IsKeyPressed(KEY_TAB))    { panel_state_.add_active = (panel_state_.add_active + 1) % kStockAddFieldCount; }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            char* buf = panel_state_.add_fields[panel_state_.add_active];
            const int len = (int)strlen(buf);
            if (len > 0) buf[len-1] = '\0';
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            char* buf = panel_state_.add_fields[panel_state_.add_active];
            const int len = (int)strlen(buf);
            if (ch >= 32 && ch <= 125 && len < 127)
                { buf[len] = (char)ch; buf[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            handle_portfolio_panel_click(GetMousePosition(), sw, sh);
        return;
    }

    // ── 3. Portfolio panel mouse clicks ────────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        handle_portfolio_panel_click(GetMousePosition(), sw, sh);
    }

    // ── 4. Save + Refresh buttons (header top-right) ──────────────────────────
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        constexpr int kBtnW = 90, kRefW = 80, kBtnH = 36, kBtnGap = 8, btn_y = 26;
        const int btn_x = sw - kMargin - kBtnW;
        const int ref_x = btn_x - kRefW - kBtnGap;
        const Vector2 m = GetMousePosition();

        if (m.x >= btn_x && m.x <= btn_x + kBtnW && m.y >= btn_y && m.y <= btn_y + kBtnH) {
            if (!file_path_.empty())
                PortfolioFile::save(portfolio_, file_path_);
            else {
                save_popup_.open = true;
                save_popup_.error.clear();
                memset(save_popup_.filename, 0, sizeof(save_popup_.filename));
            }
            return;
        }

        if (m.x >= ref_x && m.x <= ref_x + kRefW && m.y >= btn_y && m.y <= btn_y + kBtnH) {
            if (!refresher_.is_busy() && !file_path_.empty()) {
                refresh_consumed_ = false;
                refresher_.start(file_path_);
            }
            return;
        }
    }

    // ── 5. Add-position panel (bottom tab) ────────────────────────────────────
    if (IsKeyPressed(KEY_N)) {
        panel_.open = !panel_.open;
        panel_.active_field = 0;
        panel_.error_message.clear();
        return;
    }
    if (panel_.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { panel_.open = false; panel_.error_message.clear(); return; }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const Vector2 m     = GetMousePosition();
            const int tab_h     = panel_tab_height(sh);
            const int panel_top = sh - panel_reserved(panel_, sh);
            if (m.y >= panel_top && m.y < panel_top + tab_h)
                { panel_.open = false; panel_.error_message.clear(); return; }
            const int form_h  = panel_form_height(sh);
            const int form_top = panel_top + tab_h;
            const int col_gap  = 18;
            const int col_w    = (sw - kMargin * 2 - col_gap * 2) / 3;
            const int pad_y    = std::clamp(form_h * 9 / 100, 10, 18);
            const int lbl_font = std::clamp(form_h / 13, 13, 18);
            const int lbl_gap  = std::max(3, form_h / 60);
            const int field_h  = std::clamp(form_h * 22 / 100, 34, 50);
            const int row_gap  = std::clamp(form_h * 8 / 100, 8, 18);
            const int col_x[3] = { kMargin, kMargin + col_w + col_gap, kMargin + (col_w + col_gap) * 2 };
            const int row_y[2] = { form_top + pad_y, form_top + pad_y + lbl_font + lbl_gap + field_h + row_gap };
            const int btn_w    = std::clamp(sw / 12, 80, 120);
            const int btn_x    = sw - kMargin - btn_w;
            const int bot_y    = row_y[1] + lbl_font + lbl_gap + field_h + std::max(4, lbl_gap);
            const int btn_h    = std::clamp(field_h * 80 / 100, 28, 42);
            if (m.x >= btn_x - 48 && m.x <= btn_x + btn_w + 8 && m.y >= bot_y - 6 && m.y <= bot_y + btn_h + 6)
                { try_submit(); return; }
            for (int i = 0; i < kFieldCount; ++i) {
                const int col = i % 3, row = i / 3;
                const int fx = col_x[col], fy = row_y[row] + lbl_font + lbl_gap;
                if (m.x >= fx && m.x <= fx + col_w && m.y >= fy && m.y <= fy + field_h)
                    { panel_.active_field = i; return; }
            }
        }
        if (IsKeyPressed(KEY_TAB)) panel_.active_field = (panel_.active_field + 1) % kFieldCount;
        int ch = GetCharPressed();
        while (ch > 0) {
            char* buf = panel_.fields[panel_.active_field];
            const int len = (int)strlen(buf);
            if (ch >= 32 && ch <= 125 && len < 127) { buf[len] = (char)ch; buf[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsKeyDown(KEY_BACKSPACE)) {
            backspace_timer_ += GetFrameTime();
            const bool ip = IsKeyPressed(KEY_BACKSPACE);
            const bool rp = backspace_timer_ > 0.4f && std::fmod(backspace_timer_, 0.06f) < GetFrameTime();
            if (ip || rp) {
                char* buf = panel_.fields[panel_.active_field];
                const int len = (int)strlen(buf);
                if (len > 0) buf[len-1] = '\0';
            }
        } else { backspace_timer_ = 0.f; }
        if (IsKeyPressed(KEY_ENTER)) try_submit();
        return;
    }

    // ── 6. Scroll + open tab by clicking it ───────────────────────────────────
    scroll_offset_ -= GetMouseWheelMove() * 34.f;
    if (IsKeyDown(KEY_DOWN))  scroll_offset_ += 8.f;
    if (IsKeyDown(KEY_UP))    scroll_offset_ -= 8.f;
    if (IsKeyPressed(KEY_HOME)) scroll_offset_ = 0.f;

    const float max_offset = renderer_.max_scroll_offset(portfolio_, sh, panel_, panel_state_);
    scroll_offset_ = std::clamp(scroll_offset_, 0.f, max_offset);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 m = GetMousePosition();
        const int panel_top = sh - panel_tab_height(sh);
        if (m.y >= panel_top)
            { panel_.open = true; panel_.active_field = 0; panel_.error_message.clear(); }
    }
}

// ── Portfolio panel click handler ─────────────────────────────────────────────

bool PortfolioApp::handle_portfolio_panel_click(const Vector2& mouse, int screen_w, int screen_h) {
    const int bx = kMargin;
    const int bw = screen_w - kMargin * 2;
    const int by = table_top_px();
    const int reserved = panel_reserved(panel_, screen_h);
    const int bh = screen_h - by - kMargin - reserved;

    // Only process clicks inside the panel bounds
    if (mouse.x < bx || mouse.x > bx + bw || mouse.y < by || mouse.y > by + bh)
        return false;

    // ── Title bar ────────────────────────────────────────────────────────────
    if (mouse.y < by + kPanelTitleH) {
        // "+" button on right → toggle add-stock form
        if (mouse.x >= bx + bw - 44) {
            panel_state_.add_open = !panel_state_.add_open;
            if (panel_state_.add_open) {
                panel_state_.add_active = 0;
                for (int i = 0; i < kStockAddFieldCount; ++i)
                    memset(panel_state_.add_fields[i], 0, 128);
                panel_state_.error.clear();
            }
        }
        return true;
    }

    // ── Add-stock form (fixed below title) ───────────────────────────────────
    int list_top = by + kPanelTitleH;
    if (panel_state_.add_open) {
        if (mouse.y < list_top + kAddStockFormH) {
            // Compute field layout matching renderer
            const int col_gap = kAsGap;
            const int col_w = (bw - kAsPad * 2 - col_gap * 2 - 110) / 3;
            const int f_x[3] = { bx + kAsPad, bx + kAsPad + col_w + col_gap,
                                  bx + kAsPad + (col_w + col_gap) * 2 };
            const int fld_y = list_top + 16 + kAsLblFont + 4;

            for (int i = 0; i < kStockAddFieldCount; ++i) {
                if (mouse.x >= f_x[i] && mouse.x <= f_x[i] + col_w &&
                    mouse.y >= fld_y && mouse.y <= fld_y + kAsFldH) {
                    panel_state_.add_active = i;
                    return true;
                }
            }
            // "Add Stock" button
            const int btn_x = f_x[2] + col_w + col_gap;
            const int btn_w = bx + bw - kAsPad - btn_x;
            if (mouse.x >= btn_x && mouse.x <= btn_x + btn_w &&
                mouse.y >= fld_y && mouse.y <= fld_y + kAsFldH) {
                try_add_stock();
                return true;
            }
            return true;
        }
        list_top += kAddStockFormH;
    }

    // ── Column header (no action) ────────────────────────────────────────────
    if (mouse.y < list_top + kPanelColHdrH) return true;
    list_top += kPanelColHdrH;

    // ── Scrollable list ───────────────────────────────────────────────────────
    const PanelCols c = make_cols(bx, bw);
    const PosCols pc = make_pos_cols(bx, bw);
    const auto& stocks = portfolio_.stocks();
    int y = list_top - (int)scroll_offset_;

    for (int i = 0; i < (int)stocks.size(); ++i) {
        // Stock row
        if (mouse.y >= y && mouse.y < y + kStockRowH) {
            // Delete button
            if (mouse.x >= c.del_x && mouse.x <= c.del_x + 22) {
                portfolio_.remove_stock_at((std::size_t)i);
                // Clear any state referencing this stock
                if (panel_state_.selected_stock >= (int)portfolio_.stocks().size())
                    panel_state_.selected_stock = -1;
                panel_state_.editing_price = false;
                panel_state_.editing_stock_idx = -1;
                panel_state_.editing_position_idx = -1;
                memset(panel_state_.price_buf, 0, sizeof(panel_state_.price_buf));
                memset(panel_state_.edit_buf,  0, sizeof(panel_state_.edit_buf));
                if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
                return true;
            }
            // Arrow column → toggle expand
            if (mouse.x >= bx && mouse.x < c.sym_x) {
                const std::string& sym = stocks[i].symbol();
                if (panel_state_.expanded.count(sym)) panel_state_.expanded.erase(sym);
                else                                  panel_state_.expanded.insert(sym);
                return true;
            }
            // Current price column (click to select, second click to edit)
            if (mouse.x >= c.cur_x && mouse.x < c.amt_x) {
                if (panel_state_.selected_stock == i && !panel_state_.editing_price) {
                    panel_state_.editing_price = true;
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << stocks[i].current_price();
                    strncpy(panel_state_.price_buf, oss.str().c_str(), 63);
                    panel_state_.price_buf[63] = '\0';
                } else {
                    panel_state_.selected_stock = i;
                    panel_state_.editing_price  = false;
                    memset(panel_state_.price_buf, 0, sizeof(panel_state_.price_buf));
                }
                // Cancel any position edit on stock row click
                panel_state_.editing_position_idx = -1;
                panel_state_.editing_stock_idx    = -1;
                panel_state_.error.clear();
                return true;
            }
            // Any other click on stock row — just select
            panel_state_.selected_stock = i;
            panel_state_.editing_price  = false;
            panel_state_.editing_position_idx = -1;
            panel_state_.editing_stock_idx    = -1;
            panel_state_.error.clear();
            return true;
        }
        y += kStockRowH;

        // Position sub-rows (only if expanded)
        if (panel_state_.expanded.count(stocks[i].symbol())) {
            // "+" in sub-header opens add-position panel pre-filled with this symbol
            if (mouse.y >= y && mouse.y < y + kPanelColHdrH) {
                if (mouse.x >= bx + bw - 65) {
                    panel_.open = true;
                    panel_.active_field = 0;
                    panel_.error_message.clear();
                    strncpy(panel_.fields[0], stocks[i].symbol().c_str(), 127);
                    panel_.fields[0][127] = '\0';
                }
                return true;
            }
            y += kPanelColHdrH;  // Skip the position sub-header row
            for (std::size_t idx : portfolio_.position_indices_for(stocks[i].symbol())) {
                if (mouse.y >= y && mouse.y < y + kPosRowH) {
                    // Delete
                    if (mouse.x >= pc.del_x && mouse.x <= pc.del_x + 22) {
                        if (panel_state_.editing_position_idx == (int)idx)
                            { panel_state_.editing_position_idx = -1; panel_state_.editing_stock_idx = -1; }
                        portfolio_.remove_position_at(idx);
                        if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
                        return true;
                    }
                    // Amount column
                    if (mouse.x >= pc.amt_x && mouse.x < pc.buy_x) {
                        panel_state_.editing_stock_idx    = i;
                        panel_state_.editing_position_idx = (int)idx;
                        panel_state_.editing_field        = PositionEditField::Amount;
                        panel_state_.editing_price        = false;
                        panel_state_.error.clear();
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(2) << portfolio_.positions()[idx].amount();
                        strncpy(panel_state_.edit_buf, oss.str().c_str(), 127);
                        panel_state_.edit_buf[127] = '\0';
                        return true;
                    }
                    // Buy price column
                    if (mouse.x >= pc.buy_x && mouse.x < pc.val_x) {
                        panel_state_.editing_stock_idx    = i;
                        panel_state_.editing_position_idx = (int)idx;
                        panel_state_.editing_field        = PositionEditField::BuyPrice;
                        panel_state_.editing_price        = false;
                        panel_state_.error.clear();
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(2) << portfolio_.positions()[idx].buy_price();
                        strncpy(panel_state_.edit_buf, oss.str().c_str(), 127);
                        panel_state_.edit_buf[127] = '\0';
                        return true;
                    }
                    // Date column
                    if (mouse.x >= pc.date_x && mouse.x < pc.amt_x) {
                        panel_state_.editing_stock_idx    = i;
                        panel_state_.editing_position_idx = (int)idx;
                        panel_state_.editing_field        = PositionEditField::Date;
                        panel_state_.editing_price        = false;
                        panel_state_.error.clear();
                        strncpy(panel_state_.edit_buf,
                                portfolio_.positions()[idx].acquisition_date().c_str(), 127);
                        panel_state_.edit_buf[127] = '\0';
                        return true;
                    }
                    // Other click — cancel position edit
                    panel_state_.editing_position_idx = -1;
                    panel_state_.editing_stock_idx    = -1;
                    panel_state_.error.clear();
                    return true;
                }
                y += kPosRowH;
            }
        }
    }

    // Click inside panel but below all rows — cancel any active edit
    panel_state_.editing_position_idx = -1;
    panel_state_.editing_stock_idx    = -1;
    panel_state_.selected_stock       = -1;
    panel_state_.editing_price        = false;
    panel_state_.error.clear();
    return true;
}

// ── Update / Render ───────────────────────────────────────────────────────────

void PortfolioApp::update() {
    const float max_scroll = renderer_.max_scroll_offset(portfolio_, GetScreenHeight(), panel_, panel_state_);
    scroll_offset_       = std::clamp(scroll_offset_, 0.f, max_scroll);
    save_popup_.file_set = !file_path_.empty();

    // Startup auto-refresh: fires once as soon as a CSV file is available.
    if (!did_startup_refresh_ && !file_path_.empty()) {
        did_startup_refresh_ = true;
        refresh_consumed_    = false;
        refresher_.start(file_path_);
    }

    refresher_.poll();

    // Sync refresh_state_ from refresher_ each frame.
    refresh_state_.message = refresher_.last_message();
    switch (refresher_.state()) {
        case PriceRefresher::State::Idle:
            refresh_state_.status = RefreshState::Status::Idle;
            break;
        case PriceRefresher::State::Running:
            refresh_state_.status = RefreshState::Status::Running;
            break;
        case PriceRefresher::State::Succeeded:
            refresh_state_.status = RefreshState::Status::Succeeded;
            if (!refresh_consumed_) {
                PortfolioFile::load_stock_prices_only(file_path_, portfolio_);
                refresh_consumed_ = true;
            }
            break;
        case PriceRefresher::State::Failed:
            refresh_state_.status = RefreshState::Status::Failed;
            refresh_consumed_     = true;
            break;
    }
}

void PortfolioApp::render() const {
    BeginDrawing();
    ClearBackground(Color{12, 30, 48, 255});
    renderer_.draw(portfolio_, scroll_offset_, panel_, save_popup_, panel_state_, refresh_state_);
    EndDrawing();
}

// ── Try-* actions ─────────────────────────────────────────────────────────────

void PortfolioApp::try_submit() {
    const char* sym    = panel_.fields[0];
    const char* bprice = panel_.fields[1];
    const char* amt    = panel_.fields[2];
    const char* date   = panel_.fields[3];

    if (strlen(sym)  == 0) { panel_.error_message = "Symbol is required."; return; }
    if (strlen(date) == 0) { panel_.error_message = "Date is required."; return; }
    if (!portfolio_.has_stock(sym)) { panel_.error_message = "Unknown symbol. Add it in Stocks first."; return; }

    double amount = 0.0, buy_price = 0.0;
    try { amount = std::stod(amt); buy_price = std::stod(bprice); }
    catch (...) { panel_.error_message = "Amount and Buy Price must be numbers."; return; }
    if (amount    <= 0.0) { panel_.error_message = "Amount must be > 0.";    return; }
    if (buy_price <= 0.0) { panel_.error_message = "Buy Price must be > 0."; return; }

    const Stock* base = nullptr;
    for (const Stock& s : portfolio_.stocks())
        if (s.symbol() == sym) { base = &s; break; }
    if (!base) { panel_.error_message = "Could not resolve symbol."; return; }

    Stock stock(base->symbol(), base->name(), base->current_price());
    Position pos(std::move(stock), buy_price, date, amount);
    portfolio_.add_position(std::move(pos));

    for (int i = 0; i < kFieldCount; ++i) memset(panel_.fields[i], 0, sizeof(panel_.fields[i]));
    panel_.active_field = 0;
    panel_.error_message.clear();
    panel_.open = false;

    if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
}

void PortfolioApp::try_save_as() {
    if (strlen(save_popup_.filename) == 0) { save_popup_.error = "Please enter a filename."; return; }
    file_path_ = save_popup_.filename;
    PortfolioFile::save(portfolio_, file_path_);
    AppConfig cfg; cfg.last_file = file_path_;
    AppConfigIO::save(cfg);
    save_popup_.open = false;
    save_popup_.error.clear();
}

void PortfolioApp::try_add_stock() {
    const char* sym   = panel_state_.add_fields[0];
    const char* name  = panel_state_.add_fields[1];
    const char* price = panel_state_.add_fields[2];

    if (strlen(sym)  == 0) { panel_state_.error = "Symbol is required."; return; }
    if (strlen(name) == 0) { panel_state_.error = "Name is required.";   return; }

    double price_val = 0.0;
    try { price_val = std::stod(price); }
    catch (...) { panel_state_.error = "Price must be a number."; return; }
    if (price_val <= 0.0) { panel_state_.error = "Price must be > 0."; return; }

    portfolio_.add_stock(Stock(sym, name, price_val));

    for (int i = 0; i < kStockAddFieldCount; ++i)
        memset(panel_state_.add_fields[i], 0, 128);
    panel_state_.add_open   = false;
    panel_state_.add_active = 0;
    panel_state_.error.clear();

    if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
}

void PortfolioApp::try_confirm_price() {
    if (panel_state_.selected_stock < 0 ||
        panel_state_.selected_stock >= (int)portfolio_.stocks().size()) return;

    double new_price = 0.0;
    try { new_price = std::stod(panel_state_.price_buf); }
    catch (...) { panel_state_.error = "Price must be a number."; return; }
    if (new_price <= 0.0) { panel_state_.error = "Price must be > 0."; return; }

    const std::string& sym = portfolio_.stocks()[panel_state_.selected_stock].symbol();
    portfolio_.update_stock_price(sym, new_price);
    panel_state_.editing_price = false;
    panel_state_.error.clear();
    memset(panel_state_.price_buf, 0, sizeof(panel_state_.price_buf));

    if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
}

void PortfolioApp::try_confirm_position_edit() {
    if (panel_state_.editing_position_idx < 0 ||
        panel_state_.editing_position_idx >= (int)portfolio_.positions().size()) return;

    const std::size_t idx = (std::size_t)panel_state_.editing_position_idx;
    if (panel_state_.editing_field == PositionEditField::Date) {
        if (strlen(panel_state_.edit_buf) == 0) { panel_state_.error = "Date is required."; return; }
        portfolio_.update_position_date(idx, panel_state_.edit_buf);
    } else {
        double value = 0.0;
        try { value = std::stod(panel_state_.edit_buf); }
        catch (...) { panel_state_.error = "Value must be a number."; return; }
        if (value <= 0.0) { panel_state_.error = "Value must be > 0."; return; }
        if (panel_state_.editing_field == PositionEditField::Amount)
            portfolio_.update_position_amount(idx, value);
        else
            portfolio_.update_position_buy_price(idx, value);
    }

    panel_state_.editing_position_idx = -1;
    panel_state_.editing_stock_idx    = -1;
    panel_state_.error.clear();
    memset(panel_state_.edit_buf, 0, sizeof(panel_state_.edit_buf));

    if (!file_path_.empty()) PortfolioFile::save(portfolio_, file_path_);
}
