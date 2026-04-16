#ifndef PORTFOLIO_RENDERER_H
#define PORTFOLIO_RENDERER_H

#include "portfolio.h"
#include "raylib.h"

#include <algorithm>
#include <string>

// ── Add-position panel ────────────────────────────────────────────────────────

constexpr int kFieldCount = 4;  // symbol, buy price, amount, date

struct AddPanelState {
    bool open = false;
    int  active_field = 0;
    char fields[kFieldCount][128] = {};
    std::string error_message;
};

inline int panel_tab_height(int screen_height) {
    return std::clamp(screen_height / 20, 36, 52);
}
inline int panel_form_height(int screen_height) {
    return std::clamp(screen_height * 28 / 100, 180, 260);
}
inline int panel_reserved(const AddPanelState& p, int screen_height) {
    const int tab = panel_tab_height(screen_height);
    return p.open ? tab + panel_form_height(screen_height) : tab;
}

// ── Save popup ────────────────────────────────────────────────────────────────

struct SavePopupState {
    bool open     = false;
    bool file_set = false;
    char filename[512] = {};
    std::string error;
};

// ── Stock panel ───────────────────────────────────────────────────────────────

constexpr int kStockAddFieldCount = 3;  // symbol, name, price

inline int stock_panel_width(int screen_width, bool open) {
    return open ? std::clamp(screen_width * 20 / 100, 230, 310) : 36;
}

struct StockPanelState {
    bool open          = true;   // default expanded
    int  selected      = -1;
    bool editing_price = false;
    char price_buf[64] = {};

    bool add_open   = false;
    int  add_active = 0;
    char add_fields[kStockAddFieldCount][128] = {};

    std::string error;
};

enum class PositionEditField {
    Amount,
    BuyPrice,
    Date,
};

struct PositionTableState {
    int selected = -1;
    bool editing = false;
    PositionEditField editing_field = PositionEditField::Amount;
    char edit_buf[128] = {};
    std::string error;
};

// ── Renderer ──────────────────────────────────────────────────────────────────

class PortfolioRenderer {
public:
    void  draw(const Portfolio& portfolio, float scroll_offset,
               const AddPanelState& panel, const SavePopupState& save_popup,
               const StockPanelState& stock_panel,
               const PositionTableState& position_table) const;

    float max_scroll_offset(const Portfolio& portfolio, int screen_height,
                            const AddPanelState& panel) const;

private:
    void draw_header(const Portfolio& portfolio, int screen_width,
                     const SavePopupState& save_popup) const;
    void draw_summary_card(Rectangle bounds, const char* label,
                           const std::string& value, Color accent) const;
    void draw_stock_panel(const Portfolio& portfolio, Rectangle bounds,
                          const StockPanelState& sp) const;
    void draw_positions_table(const Portfolio& portfolio, Rectangle bounds,
                              float scroll_offset,
                              const PositionTableState& position_table) const;
    void draw_input_panel(const AddPanelState& panel,
                          int screen_width, int screen_height) const;
    void draw_save_popup(const SavePopupState& popup,
                         int screen_width, int screen_height) const;
    void draw_text_field(Rectangle bounds, const char* label,
                         const char* text, bool active) const;
    void draw_text_fit(const std::string& text, int x, int y,
                       int max_width, int font_size, Color color) const;

    std::string money(double value) const;
    std::string number(double value) const;
    std::string percent(double value) const;
    Color gain_color(double value) const;
};

#endif
