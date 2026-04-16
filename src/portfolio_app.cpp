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

// ── Constants shared between draw and hit-test ────────────────────────────────
static constexpr int kMargin    = 32;
static constexpr int kHdrH      = 126;  // kHeaderHeight
static constexpr int kSumH      = 110;  // kSummaryHeight
static constexpr int kTblGap    = 28;   // kTableTopGap
static constexpr int kTblHdrH   = 44;   // table header height
static constexpr int kTblRowH   = 52;   // positions row height
static constexpr int kSpHdrH    = 38;   // stock panel header height
static constexpr int kSpRowH    = 34;   // stock row height
static constexpr int kSpFrmPad  = 8;
static constexpr int kSpFldH    = 30;
static constexpr int kSpLblFont = 13;
static constexpr int kSpGap     = 8;    // gap between stock panel and table

static int table_top_px() { return kHdrH + kSumH + kTblGap; }

// ── Lifecycle ─────────────────────────────────────────────────────────────────

PortfolioApp::PortfolioApp(Portfolio portfolio, std::string file_path)
    : portfolio_(std::move(portfolio)), renderer_(), scroll_offset_(0.0f),
      panel_(), save_popup_(), stock_panel_(), position_table_(),
      backspace_timer_(0.0f), file_path_(std::move(file_path)) {
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
    const int screen_h = GetScreenHeight();
    const int screen_w = GetScreenWidth();

    // ── 1. Save popup (full modal) ─────────────────────────────────────────────
    if (save_popup_.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { save_popup_.open = false; save_popup_.error.clear(); return; }
        if (IsKeyPressed(KEY_ENTER))  { try_save_as(); return; }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            constexpr int kPopupW = 480, kPopupH = 190, kBtnW = 100, kBtnH = 36;
            const int px        = (screen_w - kPopupW) / 2;
            const int py        = (screen_h - kPopupH) / 2;
            const int confirm_x = px + kPopupW - 24 - kBtnW;
            const int cancel_x  = confirm_x - kBtnW - 12;
            const int btn_y_pos = py + kPopupH - 24 - kBtnH;
            const Vector2 m     = GetMousePosition();
            if (m.x >= confirm_x && m.x <= confirm_x + kBtnW && m.y >= btn_y_pos && m.y <= btn_y_pos + kBtnH)
                { try_save_as(); return; }
            if (m.x >= cancel_x  && m.x <= cancel_x  + kBtnW && m.y >= btn_y_pos && m.y <= btn_y_pos + kBtnH)
                { save_popup_.open = false; save_popup_.error.clear(); return; }
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            const int len = static_cast<int>(strlen(save_popup_.filename));
            if (ch >= 32 && ch <= 125 && len < 511)
                { save_popup_.filename[len] = static_cast<char>(ch); save_popup_.filename[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            const int len = static_cast<int>(strlen(save_popup_.filename));
            if (len > 0) save_popup_.filename[len-1] = '\0';
        }
        return;
    }

    // ── 2. Stock panel keyboard capture (price edit or add form) ──────────────
    if (stock_panel_.open && (stock_panel_.editing_price || stock_panel_.add_open)) {
        if (stock_panel_.editing_price) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                stock_panel_.editing_price = false;
                memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
                return;
            }
            if (IsKeyPressed(KEY_ENTER)) { try_confirm_price(); return; }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                const int len = static_cast<int>(strlen(stock_panel_.price_buf));
                if (len > 0) stock_panel_.price_buf[len-1] = '\0';
            }
            int ch = GetCharPressed();
            while (ch > 0) {
                const int len = static_cast<int>(strlen(stock_panel_.price_buf));
                if ((ch >= '0' && ch <= '9' || ch == '.') && len < 63)
                    { stock_panel_.price_buf[len] = static_cast<char>(ch); stock_panel_.price_buf[len+1] = '\0'; }
                ch = GetCharPressed();
            }
        } else {  // add form
            if (IsKeyPressed(KEY_ESCAPE)) { stock_panel_.add_open = false; stock_panel_.error.clear(); return; }
            if (IsKeyPressed(KEY_ENTER))  { try_add_stock(); return; }
            if (IsKeyPressed(KEY_TAB))    { stock_panel_.add_active = (stock_panel_.add_active + 1) % kStockAddFieldCount; }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                char* buf = stock_panel_.add_fields[stock_panel_.add_active];
                const int len = static_cast<int>(strlen(buf));
                if (len > 0) buf[len-1] = '\0';
            }
            int ch = GetCharPressed();
            while (ch > 0) {
                char* buf = stock_panel_.add_fields[stock_panel_.add_active];
                const int len = static_cast<int>(strlen(buf));
                if (ch >= 32 && ch <= 125 && len < 127)
                    { buf[len] = static_cast<char>(ch); buf[len+1] = '\0'; }
                ch = GetCharPressed();
            }
        }
        // Allow clicks through (for stock panel hit-test) but block all other keys
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const Vector2 m = GetMousePosition();
            const int spw = stock_panel_width(screen_w, stock_panel_.open);
            const int reserved = panel_reserved(panel_, screen_h);
            const int table_top = table_top_px();
            const int avail_h = screen_h - table_top - kMargin - reserved;
            const bool inside_stock_panel =
                m.x >= kMargin && m.x <= kMargin + spw &&
                m.y >= table_top && m.y <= table_top + avail_h;
            if (inside_stock_panel) {
                handle_stock_panel_click(m, screen_w, screen_h);
            } else {
                stock_panel_.editing_price = false;
                stock_panel_.selected = -1;
                memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
            }
        }
        return;
    }

    // ── 3. Positions inline edit keyboard capture ─────────────────────────────
    if (position_table_.editing) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            position_table_.editing = false;
            position_table_.error.clear();
            memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) { try_confirm_position_edit(); return; }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            const int len = static_cast<int>(strlen(position_table_.edit_buf));
            if (len > 0) position_table_.edit_buf[len - 1] = '\0';
        }
        int ch = GetCharPressed();
        while (ch > 0) {
            const int len = static_cast<int>(strlen(position_table_.edit_buf));
            if (len < 127) {
                const bool numeric_field = position_table_.editing_field != PositionEditField::Date;
                if ((numeric_field && (ch >= '0' && ch <= '9' || ch == '.')) ||
                    (!numeric_field && ch >= 32 && ch <= 125)) {
                    position_table_.edit_buf[len] = static_cast<char>(ch);
                    position_table_.edit_buf[len + 1] = '\0';
                }
            }
            ch = GetCharPressed();
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const bool consumed = handle_positions_table_click(GetMousePosition(), screen_w, screen_h);
            if (!consumed) {
                position_table_.editing = false;
                position_table_.selected = -1;
                position_table_.error.clear();
                memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
            }
        }
        return;
    }

    // ── 4. Stock panel mouse clicks (always active when panel is visible) ─────
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 m   = GetMousePosition();
        const int spw     = stock_panel_width(screen_w, stock_panel_.open);
        const int reserved = panel_reserved(panel_, screen_h);
        const int table_top = table_top_px();
        const int avail_h   = screen_h - table_top - kMargin - reserved;
        const bool inside_stock_panel =
            m.x >= kMargin && m.x <= kMargin + spw &&
            m.y >= table_top && m.y <= table_top + avail_h;
        if (inside_stock_panel) {
            if (handle_stock_panel_click(m, screen_w, screen_h)) return;
        } else if (stock_panel_.selected >= 0) {
            stock_panel_.selected = -1;
            stock_panel_.editing_price = false;
            stock_panel_.error.clear();
            memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
        }
    }

    // ── 5. Positions table mouse clicks (add/delete/select/edit) ─────────────
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (handle_positions_table_click(GetMousePosition(), screen_w, screen_h)) return;
        if (position_table_.selected >= 0 || position_table_.editing) {
            position_table_.selected = -1;
            position_table_.editing = false;
            position_table_.error.clear();
            memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
        }
    }

    // ── 6. Save button (header top-right) ─────────────────────────────────────
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        constexpr int kBtnW = 90, kBtnH = 36;
        const int btn_x = screen_w - kMargin - kBtnW;
        constexpr int btn_y = 26;
        const Vector2 m = GetMousePosition();
        if (m.x >= btn_x && m.x <= btn_x + kBtnW && m.y >= btn_y && m.y <= btn_y + kBtnH) {
            if (!file_path_.empty()) {
                PortfolioFile::save(portfolio_, file_path_);
            } else {
                save_popup_.open = true;
                save_popup_.error.clear();
                memset(save_popup_.filename, 0, sizeof(save_popup_.filename));
            }
            return;
        }
    }

    // ── 7. Add-position panel ─────────────────────────────────────────────────
    if (IsKeyPressed(KEY_N)) {
        panel_.open = !panel_.open;
        panel_.active_field = 0;
        panel_.error_message.clear();
        return;
    }

    if (panel_.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { panel_.open = false; panel_.error_message.clear(); return; }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const Vector2 m       = GetMousePosition();
            const int tab_h       = panel_tab_height(screen_h);
            const int panel_top   = screen_h - panel_reserved(panel_, screen_h);
            if (m.y >= panel_top && m.y < panel_top + tab_h) {
                panel_.open = false; panel_.error_message.clear(); return;
            }
            const int form_h  = panel_form_height(screen_h);
            const int form_top = panel_top + tab_h;
            const int col_gap  = 18;
            const int col_w    = (screen_w - kMargin * 2 - col_gap * 2) / 3;
            const int pad_y    = std::clamp(form_h * 9 / 100, 10, 18);
            const int lbl_font = std::clamp(form_h / 13, 13, 18);
            const int lbl_gap  = std::max(3, form_h / 60);
            const int field_h  = std::clamp(form_h * 22 / 100, 34, 50);
            const int row_gap  = std::clamp(form_h * 8 / 100, 8, 18);
            const int col_x[3] = { kMargin, kMargin + col_w + col_gap, kMargin + (col_w + col_gap) * 2 };
            const int row_y[2] = { form_top + pad_y, form_top + pad_y + lbl_font + lbl_gap + field_h + row_gap };
            const int btn_w    = std::clamp(screen_w / 12, 80, 120);
            const int btn_x    = screen_w - kMargin - btn_w;
            const int bot_y    = row_y[1] + lbl_font + lbl_gap + field_h + std::max(4, lbl_gap);
            const int btn_h    = std::clamp(field_h * 80 / 100, 28, 42);
            if (m.x >= btn_x - 48 && m.x <= btn_x + btn_w + 8 &&
                m.y >= bot_y - 6 && m.y <= bot_y + btn_h + 6)
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
            const int len = static_cast<int>(strlen(buf));
            if (ch >= 32 && ch <= 125 && len < 127) { buf[len] = static_cast<char>(ch); buf[len+1] = '\0'; }
            ch = GetCharPressed();
        }
        if (IsKeyDown(KEY_BACKSPACE)) {
            backspace_timer_ += GetFrameTime();
            const bool ip = IsKeyPressed(KEY_BACKSPACE);
            const bool rp = backspace_timer_ > 0.4f && std::fmod(backspace_timer_, 0.06f) < GetFrameTime();
            if (ip || rp) {
                char* buf = panel_.fields[panel_.active_field];
                const int len = static_cast<int>(strlen(buf));
                if (len > 0) buf[len-1] = '\0';
            }
        } else { backspace_timer_ = 0.0f; }
        if (IsKeyPressed(KEY_ENTER)) try_submit();
        return;
    }

    // ── 8. Normal scroll + tab bar click ──────────────────────────────────────
    scroll_offset_ -= GetMouseWheelMove() * 34.0f;
    if (IsKeyDown(KEY_DOWN))  scroll_offset_ += 8.0f;
    if (IsKeyDown(KEY_UP))    scroll_offset_ -= 8.0f;
    if (IsKeyPressed(KEY_HOME)) scroll_offset_ = 0.0f;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 m    = GetMousePosition();
        const int tab_h    = panel_tab_height(screen_h);
        const int panel_top = screen_h - tab_h;
        if (m.y >= panel_top)
            { panel_.open = true; panel_.active_field = 0; panel_.error_message.clear(); }
    }
}

// ── Stock panel click handler ─────────────────────────────────────────────────

bool PortfolioApp::handle_stock_panel_click(const Vector2& mouse, int screen_w, int screen_h) {
    const int spw = stock_panel_width(screen_w, stock_panel_.open);
    const int px  = kMargin;
    const int py  = table_top_px();
    (void)screen_h;

    if (!stock_panel_.open) {
        stock_panel_.open = true;
        return true;
    }

    // Header region
    if (mouse.y < py + kSpHdrH) {
        if (mouse.x < px + 28) {  // "<" collapse button
            stock_panel_.open          = false;
            stock_panel_.editing_price = false;
            stock_panel_.add_open      = false;
            stock_panel_.error.clear();
        } else if (mouse.x > px + spw - 32) {  // "+" add button
            stock_panel_.add_open = !stock_panel_.add_open;
            if (stock_panel_.add_open) {
                stock_panel_.add_active = 0;
                for (int i = 0; i < kStockAddFieldCount; ++i)
                    memset(stock_panel_.add_fields[i], 0, 128);
                stock_panel_.error.clear();
            }
        }
        return true;
    }

    // Stock rows
    const int rows_top = py + kSpHdrH + 1;
    const auto& stocks = portfolio_.stocks();
    for (int i = 0; i < static_cast<int>(stocks.size()); ++i) {
        const int ry = rows_top + i * kSpRowH;
        if (mouse.y >= ry && mouse.y < ry + kSpRowH) {
            const int del_x = px + spw - 24;
            const int del_y = ry + (kSpRowH - 18) / 2;
            if (mouse.x >= del_x && mouse.x <= del_x + 16 &&
                mouse.y >= del_y && mouse.y <= del_y + 18) {
                if (portfolio_.remove_stock_at(static_cast<std::size_t>(i))) {
                    if (stock_panel_.selected >= static_cast<int>(portfolio_.stocks().size()))
                        stock_panel_.selected = static_cast<int>(portfolio_.stocks().size()) - 1;
                }
                stock_panel_.editing_price = false;
                memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
                return true;
            }

            if (stock_panel_.selected == i && !stock_panel_.editing_price) {
                // Second click on same row → enter price edit
                stock_panel_.editing_price = true;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << stocks[i].current_price();
                strncpy(stock_panel_.price_buf, oss.str().c_str(), 63);
                stock_panel_.price_buf[63] = '\0';
            } else {
                stock_panel_.selected      = i;
                stock_panel_.editing_price = false;
                memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
            }
            return true;
        }
    }

    // Add-form fields / Add button
    if (stock_panel_.add_open) {
        const int form_top = rows_top + static_cast<int>(stocks.size()) * kSpRowH + 1;
        int fy = form_top + kSpFrmPad;
        for (int i = 0; i < kStockAddFieldCount; ++i) {
            fy += kSpLblFont + 3;
            if (mouse.y >= fy && mouse.y < fy + kSpFldH) { stock_panel_.add_active = i; return true; }
            fy += kSpFldH + kSpFrmPad;
        }
        if (mouse.y >= fy && mouse.y < fy + kSpFldH) { try_add_stock(); return true; }
    }

    stock_panel_.selected = -1;
    stock_panel_.editing_price = false;
    stock_panel_.error.clear();
    memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
    return true;  // inside panel, always consume
}

bool PortfolioApp::handle_positions_table_click(const Vector2& mouse, int screen_w, int screen_h) {
    const int reserved   = panel_reserved(panel_, screen_h);
    const int table_top  = table_top_px();
    const int avail_h    = screen_h - table_top - kMargin - reserved;
    const int spw        = stock_panel_width(screen_w, stock_panel_.open);
    const int left       = kMargin + spw + kSpGap;
    const int right      = screen_w - kMargin;
    const int bottom     = table_top + avail_h;
    if (mouse.x < left || mouse.x > right || mouse.y < table_top || mouse.y > bottom) return false;

    const int width      = right - left;
    const int top        = table_top - static_cast<int>(scroll_offset_);
    const int header_top = top;
    const int plus_x     = right - 34;
    const int plus_y     = header_top + 10;
    const int plus_w     = MeasureText("+", 24);
    if (mouse.x >= plus_x - 6 && mouse.x <= plus_x + plus_w + 8 &&
        mouse.y >= plus_y - 6 && mouse.y <= plus_y + 24 + 6 &&
        mouse.y >= header_top && mouse.y <= header_top + kTblHdrH) {
        panel_.open = true;
        panel_.active_field = 0;
        panel_.error_message.clear();
        return true;
    }

    const auto& positions = portfolio_.positions();
    int y = top + kTblHdrH;
    const int amount_x  = left + width * 38 / 100;
    const int buy_x     = left + width * 50 / 100;
    const int current_x = left + width * 61 / 100;
    const int date_x    = left + width * 90 / 100;
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (mouse.y >= y && mouse.y <= y + kTblRowH) {
            const int del_x = right - 34;
            const int del_y = y + (kTblRowH - 22) / 2;
            if (mouse.x >= del_x && mouse.x <= del_x + 22 &&
                mouse.y >= del_y && mouse.y <= del_y + 22) {
                portfolio_.remove_position_at(i);
                if (position_table_.selected >= static_cast<int>(portfolio_.positions().size()))
                    position_table_.selected = static_cast<int>(portfolio_.positions().size()) - 1;
                position_table_.editing = false;
                position_table_.error.clear();
                memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
                return true;
            }

            PositionEditField clicked_field = PositionEditField::Amount;
            bool has_edit_field = false;
            const int amount_l = amount_x - 4;
            const int amount_r = amount_l + std::max(90, buy_x - amount_x - 10);
            const int buy_l = buy_x - 4;
            const int buy_r = buy_l + std::max(100, current_x - buy_x - 10);
            const int date_l = date_x - 48;
            const int date_r = date_l + std::max(120, right - (date_x - 48) - 46);
            if (mouse.x >= amount_l && mouse.x <= amount_r) {
                clicked_field = PositionEditField::Amount;
                has_edit_field = true;
            } else if (mouse.x >= buy_l && mouse.x <= buy_r) {
                clicked_field = PositionEditField::BuyPrice;
                has_edit_field = true;
            } else if (mouse.x >= date_l && mouse.x <= date_r) {
                clicked_field = PositionEditField::Date;
                has_edit_field = true;
            }

            if (has_edit_field) {
                position_table_.selected = static_cast<int>(i);
                position_table_.editing = true;
                position_table_.editing_field = clicked_field;
                position_table_.error.clear();
                std::ostringstream oss;
                if (clicked_field == PositionEditField::Amount) {
                    oss << std::fixed << std::setprecision(2) << positions[i].amount();
                    strncpy(position_table_.edit_buf, oss.str().c_str(), sizeof(position_table_.edit_buf) - 1);
                    position_table_.edit_buf[sizeof(position_table_.edit_buf) - 1] = '\0';
                } else if (clicked_field == PositionEditField::BuyPrice) {
                    oss << std::fixed << std::setprecision(2) << positions[i].buy_price();
                    strncpy(position_table_.edit_buf, oss.str().c_str(), sizeof(position_table_.edit_buf) - 1);
                    position_table_.edit_buf[sizeof(position_table_.edit_buf) - 1] = '\0';
                } else {
                    strncpy(position_table_.edit_buf, positions[i].acquisition_date().c_str(),
                            sizeof(position_table_.edit_buf) - 1);
                    position_table_.edit_buf[sizeof(position_table_.edit_buf) - 1] = '\0';
                }
            } else {
                position_table_.selected = static_cast<int>(i);
                position_table_.editing = false;
                position_table_.error.clear();
                memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
            }
            return true;
        }
        y += kTblRowH;
    }

    position_table_.selected = -1;
    position_table_.editing = false;
    position_table_.error.clear();
    memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
    return true;
}

// ── Update / Render ───────────────────────────────────────────────────────────

void PortfolioApp::update() {
    const float max_scroll = renderer_.max_scroll_offset(portfolio_, GetScreenHeight(), panel_);
    scroll_offset_         = std::clamp(scroll_offset_, 0.0f, max_scroll);
    save_popup_.file_set   = !file_path_.empty();
}

void PortfolioApp::render() const {
    BeginDrawing();
    ClearBackground(Color{12, 30, 48, 255});
    renderer_.draw(portfolio_, scroll_offset_, panel_, save_popup_, stock_panel_, position_table_);
    EndDrawing();
}

// ── Try-* actions ─────────────────────────────────────────────────────────────

void PortfolioApp::try_submit() {
    const char* sym    = panel_.fields[0];
    const char* bprice = panel_.fields[1];
    const char* amt    = panel_.fields[2];
    const char* date   = panel_.fields[3];

    if (strlen(sym) == 0) { panel_.error_message = "Symbol is required."; return; }
    if (strlen(date) == 0) { panel_.error_message = "Date is required."; return; }
    if (!portfolio_.has_stock(sym)) {
        panel_.error_message = "Unknown symbol. Add it in Stocks first.";
        return;
    }

    double amount = 0.0, buy_price = 0.0;
    try {
        amount    = std::stod(amt);
        buy_price = std::stod(bprice);
    } catch (...) {
        panel_.error_message = "Amount and Buy Price must be numbers.";
        return;
    }
    if (amount    <= 0.0) { panel_.error_message = "Amount must be > 0.";         return; }
    if (buy_price <= 0.0) { panel_.error_message = "Buy Price must be > 0.";      return; }

    const Stock* base_stock = nullptr;
    for (const Stock& stock : portfolio_.stocks()) {
        if (stock.symbol() == sym) {
            base_stock = &stock;
            break;
        }
    }
    if (base_stock == nullptr) {
        panel_.error_message = "Could not resolve symbol in stock registry.";
        return;
    }

    Stock    stock(base_stock->symbol(), base_stock->name(), base_stock->current_price());
    Position pos(std::move(stock), buy_price, date, amount);
    portfolio_.add_position(std::move(pos));

    for (int i = 0; i < kFieldCount; ++i) memset(panel_.fields[i], 0, sizeof(panel_.fields[i]));
    panel_.active_field = 0;
    panel_.error_message.clear();
    panel_.open = false;
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
    const char* sym   = stock_panel_.add_fields[0];
    const char* name  = stock_panel_.add_fields[1];
    const char* price = stock_panel_.add_fields[2];

    if (strlen(sym)   == 0) { stock_panel_.error = "Symbol is required."; return; }
    if (strlen(name)  == 0) { stock_panel_.error = "Name is required.";   return; }

    double price_val = 0.0;
    try { price_val = std::stod(price); } catch (...) {
        stock_panel_.error = "Price must be a number."; return;
    }
    if (price_val <= 0.0) { stock_panel_.error = "Price must be > 0."; return; }

    portfolio_.add_stock(Stock(sym, name, price_val));

    for (int i = 0; i < kStockAddFieldCount; ++i)
        memset(stock_panel_.add_fields[i], 0, 128);
    stock_panel_.add_open   = false;
    stock_panel_.add_active = 0;
    stock_panel_.error.clear();
}

void PortfolioApp::try_confirm_price() {
    if (stock_panel_.selected < 0 ||
        stock_panel_.selected >= static_cast<int>(portfolio_.stocks().size())) return;

    double new_price = 0.0;
    try { new_price = std::stod(stock_panel_.price_buf); } catch (...) {
        stock_panel_.error = "Price must be a number."; return;
    }
    if (new_price <= 0.0) { stock_panel_.error = "Price must be > 0."; return; }

    const std::string& sym = portfolio_.stocks()[stock_panel_.selected].symbol();
    portfolio_.update_stock_price(sym, new_price);

    stock_panel_.editing_price = false;
    stock_panel_.error.clear();
    memset(stock_panel_.price_buf, 0, sizeof(stock_panel_.price_buf));
}

void PortfolioApp::try_confirm_position_edit() {
    if (position_table_.selected < 0 ||
        position_table_.selected >= static_cast<int>(portfolio_.positions().size())) return;

    const std::size_t idx = static_cast<std::size_t>(position_table_.selected);
    if (position_table_.editing_field == PositionEditField::Date) {
        if (strlen(position_table_.edit_buf) == 0) {
            position_table_.error = "Date is required.";
            return;
        }
        portfolio_.update_position_date(idx, position_table_.edit_buf);
    } else {
        double value = 0.0;
        try { value = std::stod(position_table_.edit_buf); } catch (...) {
            position_table_.error = "Value must be a number.";
            return;
        }
        if (value <= 0.0) {
            position_table_.error = "Value must be > 0.";
            return;
        }
        if (position_table_.editing_field == PositionEditField::Amount)
            portfolio_.update_position_amount(idx, value);
        else
            portfolio_.update_position_buy_price(idx, value);
    }
    position_table_.editing = false;
    position_table_.error.clear();
    memset(position_table_.edit_buf, 0, sizeof(position_table_.edit_buf));
}
