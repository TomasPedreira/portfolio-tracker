#include "portfolio_renderer.h"

#include "position.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace {
constexpr int kMargin = 32;
constexpr int kHeaderHeight = 126;
constexpr int kSummaryHeight = 110;
constexpr int kTableTopGap = 28;
constexpr int kTableHeaderHeight = 44;
constexpr int kRowHeight = 52;

constexpr Color kTitle = {232, 240, 248, 255};
constexpr Color kText = {220, 232, 242, 255};
constexpr Color kMuted = {151, 171, 190, 255};
constexpr Color kLine = {71, 98, 122, 255};
constexpr Color kPanel = {24, 52, 78, 255};
constexpr Color kPanelAlt = {30, 61, 90, 255};
constexpr Color kTableHeader = {19, 43, 68, 255};
constexpr Color kGreen = {74, 197, 139, 255};
constexpr Color kRed = {238, 103, 112, 255};
constexpr Color kBlue = {89, 153, 224, 255};
constexpr Color kInk = {137, 178, 222, 255};
constexpr Color kShadow = {5, 15, 25, 95};
constexpr Color kPanelDark = {14, 34, 56, 255};

int table_content_height(const Portfolio& portfolio) {
    return kTableHeaderHeight + static_cast<int>(portfolio.positions().size()) * kRowHeight;
}

// Form layout helpers — all proportional to screen dimensions.
// These mirror the drawing logic so hit-tests in the app stay in sync.

struct FormMetrics {
    int pad_x;
    int pad_y;
    int col_width;
    int col_gap;
    int label_font;
    int label_gap;
    int field_h;
    int row_gap;
    int btn_w;
    int col_x[3];
    int row_y[2];   // top-left y of the label for each row
    int form_top;
};

FormMetrics compute_form(int screen_width, int screen_height, int form_top, int form_height) {
    FormMetrics m{};
    m.form_top   = form_top;
    m.pad_x      = kMargin;
    m.pad_y      = std::clamp(form_height * 9 / 100, 10, 18);
    m.col_gap    = 18;
    m.col_width  = (screen_width - m.pad_x * 2 - m.col_gap * 2) / 3;
    m.label_font = std::clamp(form_height / 13, 13, 18);
    m.label_gap  = std::max(3, form_height / 60);
    m.field_h    = std::clamp(form_height * 22 / 100, 34, 50);
    m.row_gap    = std::clamp(form_height * 8 / 100, 8, 18);
    m.btn_w      = std::clamp(screen_width / 12, 80, 120);

    m.col_x[0] = m.pad_x;
    m.col_x[1] = m.pad_x + m.col_width + m.col_gap;
    m.col_x[2] = m.pad_x + (m.col_width + m.col_gap) * 2;

    m.row_y[0] = form_top + m.pad_y;
    m.row_y[1] = m.row_y[0] + m.label_font + m.label_gap + m.field_h + m.row_gap;

    return m;
}
}

void PortfolioRenderer::draw(const Portfolio& portfolio, float scroll_offset,
                             const AddPanelState& panel, const SavePopupState& save_popup,
                             const StockPanelState& stock_panel,
                             const PositionTableState& position_table) const {
    const int screen_width  = GetScreenWidth();
    const int screen_height = GetScreenHeight();

    draw_header(portfolio, screen_width, save_popup);

    // Summary cards — full width, above the split
    const int summary_top = kHeaderHeight;
    const int card_width  = (screen_width - kMargin * 2 - 36) / 3;
    draw_summary_card(Rectangle{static_cast<float>(kMargin), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Invested", money(portfolio.total_cost_basis()), kBlue);
    draw_summary_card(Rectangle{static_cast<float>(kMargin + card_width + 18), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Current value", money(portfolio.total_current_value()), kInk);
    draw_summary_card(Rectangle{static_cast<float>(kMargin + (card_width + 18) * 2), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Gain / loss",
                      money(portfolio.total_gain_loss()) + "  " + percent(portfolio.total_gain_loss_percent()),
                      gain_color(portfolio.total_gain_loss()));

    // Split area: stock panel (left) + positions table (right)
    const int reserved    = panel_reserved(panel, screen_height);
    const int table_top   = summary_top + kSummaryHeight + kTableTopGap;
    const int avail_h     = screen_height - table_top - kMargin - reserved;
    constexpr int kGap    = 8;  // gap between stock panel and table
    const int spw         = stock_panel_width(screen_width, stock_panel.open);

    const Rectangle sp_bounds{
        static_cast<float>(kMargin), static_cast<float>(table_top),
        static_cast<float>(spw),     static_cast<float>(avail_h),
    };
    draw_stock_panel(portfolio, sp_bounds, stock_panel);

    const Rectangle table_bounds{
        static_cast<float>(kMargin + spw + kGap), static_cast<float>(table_top),
        static_cast<float>(screen_width - kMargin - spw - kGap - kMargin),
        static_cast<float>(avail_h),
    };
    draw_positions_table(portfolio, table_bounds, scroll_offset, position_table);

    draw_input_panel(panel, screen_width, screen_height);

    if (save_popup.open)
        draw_save_popup(save_popup, screen_width, screen_height);
}

float PortfolioRenderer::max_scroll_offset(const Portfolio& portfolio, int screen_height, const AddPanelState& panel) const {
    const int table_top = kHeaderHeight + kSummaryHeight + kTableTopGap;
    const int visible_height = screen_height - table_top - kMargin - panel_reserved(panel, screen_height);
    const int overflow = table_content_height(portfolio) - visible_height;
    return static_cast<float>(std::max(0, overflow));
}

void PortfolioRenderer::draw_header(const Portfolio& portfolio, int screen_width,
                                    const SavePopupState& save_popup) const {
    DrawText("Portfolio Tracker", kMargin, 24, 40, kTitle);

    const std::string subtitle = std::to_string(portfolio.positions().size()) + " open positions";
    DrawText(subtitle.c_str(), kMargin, 66, 22, kMuted);

    DrawLineEx(Vector2{static_cast<float>(kMargin), 88.0f},
               Vector2{static_cast<float>(screen_width - kMargin), 88.0f}, 2.0f, kLine);

    // Save button — top-right of header
    constexpr int kBtnW = 90;
    constexpr int kBtnH = 36;
    const int btn_x = screen_width - kMargin - kBtnW;
    constexpr int btn_y = 26;

    const Color btn_bg  = save_popup.file_set ? kBlue : kInk;
    DrawRectangleRounded(Rectangle{static_cast<float>(btn_x), static_cast<float>(btn_y),
                                   kBtnW, kBtnH}, 0.2f, 8, btn_bg);
    const char* btn_label = save_popup.file_set ? "Save" : "Save as";
    const int lw = MeasureText(btn_label, 18);
    DrawText(btn_label, btn_x + (kBtnW - lw) / 2, btn_y + (kBtnH - 18) / 2, 18, kTitle);
}

void PortfolioRenderer::draw_summary_card(Rectangle bounds, const char* label, const std::string& value, Color accent) const {
    DrawRectangleRounded(Rectangle{bounds.x + 4.0f, bounds.y + 6.0f, bounds.width, bounds.height},
                         0.08f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.08f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.08f, 8, kLine);
    DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y), 5, static_cast<int>(bounds.height), accent);

    DrawText(label, static_cast<int>(bounds.x) + 22, static_cast<int>(bounds.y) + 22, 20, kMuted);
    draw_text_fit(value, static_cast<int>(bounds.x) + 22, static_cast<int>(bounds.y) + 58,
                  static_cast<int>(bounds.width) - 44, 30, kTitle);
}

void PortfolioRenderer::draw_positions_table(const Portfolio& portfolio, Rectangle bounds, float scroll_offset,
                                             const PositionTableState& position_table) const {
    DrawRectangleRounded(Rectangle{bounds.x + 5.0f, bounds.y + 7.0f, bounds.width, bounds.height},
                         0.04f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.04f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.04f, 8, kLine);

    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y),
                     static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    const int left = static_cast<int>(bounds.x);
    const int right = left + static_cast<int>(bounds.width);
    const int top = static_cast<int>(bounds.y) - static_cast<int>(scroll_offset);
    const int width = right - left;
    const int symbol_x = left + 20;
    const int name_x = left + width * 10 / 100;
    const int amount_x = left + width * 38 / 100;
    const int buy_x = left + width * 50 / 100;
    const int current_x = left + width * 61 / 100;
    const int value_x = left + width * 72 / 100;
    const int gain_x = left + width * 82 / 100;
    const int date_x = left + width * 90 / 100;
    const Vector2 mouse = GetMousePosition();

    DrawRectangle(left, top, static_cast<int>(bounds.width), kTableHeaderHeight, kTableHeader);
    DrawText("Symbol", symbol_x, top + 13, 20, kMuted);
    DrawText("Name", name_x, top + 13, 20, kMuted);
    DrawText("Amount", amount_x, top + 13, 20, kMuted);
    DrawText("Buy", buy_x, top + 13, 20, kMuted);
    DrawText("Current", current_x, top + 13, 20, kMuted);
    DrawText("Value", value_x, top + 13, 20, kMuted);
    DrawText("Gain", gain_x, top + 13, 20, kMuted);
    DrawText("Date", date_x, top + 13, 20, kMuted);
    DrawText("+", right - 34, top + 10, 24, kBlue);
    DrawLineEx(Vector2{static_cast<float>(left), static_cast<float>(top + kTableHeaderHeight)},
               Vector2{static_cast<float>(left + static_cast<int>(bounds.width)), static_cast<float>(top + kTableHeaderHeight)},
               2.0f, kLine);

    int y = top + kTableHeaderHeight;
    const auto& positions = portfolio.positions();
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& position = positions[i];
        const Color row_color = ((y / kRowHeight) % 2 == 0) ? kPanel : kPanelAlt;
        const bool hovered = mouse.x >= left && mouse.x <= right &&
                             mouse.y >= y && mouse.y <= y + kRowHeight;
        DrawRectangle(left, y, static_cast<int>(bounds.width), kRowHeight, row_color);
        DrawLine(left, y + kRowHeight, right, y + kRowHeight, kLine);

        DrawText(position.stock().symbol().c_str(), symbol_x, y + 15, 21, kTitle);
        draw_text_fit(position.stock().name(), name_x, y + 15, amount_x - name_x - 18, 21, kText);
        const bool is_edit_row = position_table.editing && position_table.selected == static_cast<int>(i);
        if (is_edit_row && position_table.editing_field == PositionEditField::Amount) {
            draw_text_field(Rectangle{static_cast<float>(amount_x - 4), static_cast<float>(y + 9),
                                      static_cast<float>(std::max(90, buy_x - amount_x - 10)), 34.0f},
                            "", position_table.edit_buf, true);
        } else {
            DrawText(number(position.amount()).c_str(), amount_x, y + 15, 21, kText);
        }
        if (is_edit_row && position_table.editing_field == PositionEditField::BuyPrice) {
            draw_text_field(Rectangle{static_cast<float>(buy_x - 4), static_cast<float>(y + 9),
                                      static_cast<float>(std::max(100, current_x - buy_x - 10)), 34.0f},
                            "", position_table.edit_buf, true);
        } else {
            DrawText(money(position.buy_price()).c_str(), buy_x, y + 15, 21, kText);
        }
        DrawText(money(position.stock().current_price()).c_str(), current_x, y + 15, 21, kText);
        DrawText(money(position.current_value()).c_str(), value_x, y + 15, 21, kText);
        DrawText(percent(position.gain_loss_percent()).c_str(), gain_x, y + 15, 21, gain_color(position.gain_loss()));
        if (is_edit_row && position_table.editing_field == PositionEditField::Date) {
            draw_text_field(Rectangle{static_cast<float>(date_x - 48), static_cast<float>(y + 9),
                                      static_cast<float>(std::max(120, right - (date_x - 48) - 46)), 34.0f},
                            "", position_table.edit_buf, true);
        } else {
            DrawText(position.acquisition_date().c_str(), date_x - 44, y + 15, 21, kText);
        }
        if (hovered) {
            const Rectangle del_box{
                static_cast<float>(right - 34),
                static_cast<float>(y + (kRowHeight - 22) / 2),
                22.0f,
                22.0f,
            };
            DrawRectangleRounded(del_box, 0.25f, 6, Color{161, 61, 70, 255});
            DrawText("x", static_cast<int>(del_box.x) + 6, static_cast<int>(del_box.y) + 2, 18, kTitle);
        }

        y += kRowHeight;
    }

    EndScissorMode();

    if (!position_table.error.empty()) {
        DrawText(position_table.error.c_str(),
                 static_cast<int>(bounds.x) + 12,
                 static_cast<int>(bounds.y + bounds.height) - 24, 14, kRed);
    }
}

void PortfolioRenderer::draw_input_panel(const AddPanelState& panel, int screen_width, int screen_height) const {
    const int tab_h    = panel_tab_height(screen_height);
    const int reserved = panel_reserved(panel, screen_height);
    const int panel_top = screen_height - reserved;

    // Background
    DrawRectangle(0, panel_top, screen_width, reserved, kPanelDark);
    DrawLineEx(Vector2{0.0f, static_cast<float>(panel_top)},
               Vector2{static_cast<float>(screen_width), static_cast<float>(panel_top)},
               2.0f, kLine);

    // Tab bar
    const int tab_font = std::clamp(tab_h * 45 / 100, 14, 22);
    const int tab_text_y = panel_top + (tab_h - tab_font) / 2;
    const Color tab_color = panel.open ? kBlue : kInk;
    DrawText("+ Add Position", kMargin, tab_text_y, tab_font, tab_color);

    if (!panel.open) {
        const char* hint = "N  to open  |  scroll: wheel / Up / Down  |  Home: top";
        const int hint_w = MeasureText(hint, tab_font - 2);
        DrawText(hint, screen_width - kMargin - hint_w, tab_text_y + 1, tab_font - 2, kMuted);
        return;
    }

    // Collapse hint on the right of the tab bar when open
    const char* close_hint = "Esc to close";
    const int close_w = MeasureText(close_hint, tab_font - 2);
    DrawText(close_hint, screen_width - kMargin - close_w, tab_text_y + 1, tab_font - 2, kMuted);

    // Form area
    const int form_h   = panel_form_height(screen_height);
    const int form_top = panel_top + tab_h;
    const FormMetrics m = compute_form(screen_width, screen_height, form_top, form_h);

    static const char* labels[kFieldCount] = {
        "Symbol", "Buy Price", "Amount", "Date (YYYY-MM-DD)"
    };

    for (int i = 0; i < kFieldCount; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const int lx  = m.col_x[col];
        const int ly  = m.row_y[row];
        const Rectangle field_rect{
            static_cast<float>(lx),
            static_cast<float>(ly + m.label_font + m.label_gap),
            static_cast<float>(m.col_width),
            static_cast<float>(m.field_h),
        };
        DrawText(labels[i], lx, ly, m.label_font, kMuted);
        draw_text_field(field_rect, labels[i], panel.fields[i], i == panel.active_field);
    }

    // Bottom row: error/hint on the left, Add button on the right
    const int bottom_y   = m.row_y[1] + m.label_font + m.label_gap + m.field_h + std::max(4, m.label_gap);
    const int btn_h      = std::clamp(m.field_h * 80 / 100, 28, 42);
    const int btn_right  = screen_width - kMargin;
    const Rectangle btn_rect{
        static_cast<float>(btn_right - m.btn_w),
        static_cast<float>(bottom_y),
        static_cast<float>(m.btn_w),
        static_cast<float>(btn_h),
    };
    DrawRectangleRounded(btn_rect, 0.2f, 8, kBlue);
    const char* btn_label = "Add";
    const int btn_font    = std::clamp(btn_h * 50 / 100, 13, 20);
    const int btn_lw      = MeasureText(btn_label, btn_font);
    DrawText(btn_label,
             static_cast<int>(btn_rect.x) + (m.btn_w - btn_lw) / 2,
             static_cast<int>(btn_rect.y) + (btn_h - btn_font) / 2,
             btn_font, kTitle);

    const int msg_y = bottom_y + (btn_h - m.label_font) / 2;
    if (!panel.error_message.empty()) {
        DrawText(panel.error_message.c_str(), kMargin, msg_y, m.label_font, kRed);
    } else {
        DrawText("Tab / click to switch fields  |  Enter to add", kMargin, msg_y, m.label_font - 1, kMuted);
    }
}

void PortfolioRenderer::draw_text_field(Rectangle bounds, const char* /*label*/, const char* text, bool active) const {
    const Color border = active ? kBlue : kLine;
    const float thickness = active ? 2.0f : 1.0f;

    DrawRectangleRounded(bounds, 0.12f, 6, kPanel);
    DrawRectangleRoundedLines(bounds, 0.12f, 6, border);
    // DrawRectangleRoundedLines doesn't take thickness in older raylib; draw a second pass for active
    if (active) {
        DrawRectangleRoundedLines(Rectangle{bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2}, 0.12f, 6, kBlue);
    }
    (void)thickness;

    const int font_size = std::clamp(static_cast<int>(bounds.height) * 45 / 100, 14, 22);
    const int pad_x     = std::max(8, static_cast<int>(bounds.width) * 3 / 100);
    const int text_x    = static_cast<int>(bounds.x) + pad_x;
    const int text_y    = static_cast<int>(bounds.y) + (static_cast<int>(bounds.height) - font_size) / 2;

    DrawText(text, text_x, text_y, font_size, kTitle);

    if (active && static_cast<int>(GetTime() * 2) % 2 == 0) {
        const int cursor_x = text_x + MeasureText(text, font_size);
        DrawRectangle(cursor_x + 1, text_y, 2, font_size, kBlue);
    }
}

void PortfolioRenderer::draw_stock_panel(const Portfolio& portfolio,
                                          Rectangle bounds,
                                          const StockPanelState& sp) const {
    const int bx = static_cast<int>(bounds.x);
    const int by = static_cast<int>(bounds.y);
    const int bw = static_cast<int>(bounds.width);
    const int bh = static_cast<int>(bounds.height);

    // Background
    DrawRectangleRounded(Rectangle{bounds.x + 4.f, bounds.y + 6.f, bounds.width, bounds.height},
                         0.04f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.04f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.04f, 8, kLine);

    if (!sp.open) {
        // Collapsed — draw expand arrow centred in the strip
        DrawText(">", bx + (bw - MeasureText(">", 18)) / 2, by + 14, 18, kInk);
        return;
    }

    // ── Header ────────────────────────────────────────────────────────────────
    constexpr int kHdrH = 38;
    DrawRectangle(bx, by, bw, kHdrH, kTableHeader);

    DrawText("<", bx + 8, by + (kHdrH - 18) / 2, 18, kMuted);

    const char* title = "Stocks";
    DrawText(title, bx + (bw - MeasureText(title, 16)) / 2, by + (kHdrH - 16) / 2, 16, kTitle);

    const char* plus = "+";
    const int plus_x = bx + bw - 26;
    DrawText(plus, plus_x, by + (kHdrH - 20) / 2, 20, kBlue);

    DrawLineEx(Vector2{static_cast<float>(bx), static_cast<float>(by + kHdrH)},
               Vector2{static_cast<float>(bx + bw), static_cast<float>(by + kHdrH)}, 1.f, kLine);

    // ── Stock list ─────────────────────────────────────────────────────────────
    constexpr int kRowH   = 34;
    constexpr int kSymFont = 15;
    constexpr int kPriceFont = 14;
    constexpr int kPad    = 10;

    const auto& stocks = portfolio.stocks();
    const int rows_top = by + kHdrH + 1;
    const Vector2 mouse = GetMousePosition();

    BeginScissorMode(bx, by + kHdrH, bw, bh - kHdrH);

    for (int i = 0; i < static_cast<int>(stocks.size()); ++i) {
        const int ry       = rows_top + i * kRowH;
        const bool sel     = (i == sp.selected);
        const bool hovered = mouse.x >= bx && mouse.x <= bx + bw &&
                             mouse.y >= ry && mouse.y <= ry + kRowH;
        DrawRectangle(bx, ry, bw, kRowH, sel ? kPanelAlt : kPanel);
        DrawLineEx(Vector2{static_cast<float>(bx), static_cast<float>(ry + kRowH)},
                   Vector2{static_cast<float>(bx + bw), static_cast<float>(ry + kRowH)}, 1.f, kLine);

        draw_text_fit(stocks[i].symbol(), bx + kPad, ry + (kRowH - kSymFont) / 2,
                      bw / 2 - kPad, kSymFont, sel ? kBlue : kTitle);

        if (sel && sp.editing_price) {
            // Inline price input on the right half
            const int fw = bw / 2 - kPad;
            const int fx = bx + bw - fw - kPad;
            draw_text_field(Rectangle{static_cast<float>(fx), static_cast<float>(ry + 5),
                                      static_cast<float>(fw), static_cast<float>(kRowH - 10)},
                            "", sp.price_buf, true);
        } else {
            const std::string ps = number(stocks[i].current_price());
            const int pw = MeasureText(ps.c_str(), kPriceFont);
            const int price_x = bx + bw - pw - kPad - 28;
            DrawText(ps.c_str(), price_x, ry + (kRowH - kPriceFont) / 2,
                     kPriceFont, sel ? kBlue : kText);
        }

        if (hovered) {
            const Rectangle del_box{
                static_cast<float>(bx + bw - 24),
                static_cast<float>(ry + (kRowH - 18) / 2),
                16.0f,
                18.0f,
            };
            DrawRectangleRounded(del_box, 0.3f, 6, Color{161, 61, 70, 255});
            DrawText("x", static_cast<int>(del_box.x) + 4, static_cast<int>(del_box.y) + 1, 14, kTitle);
        }
    }

    // ── Add-stock form ─────────────────────────────────────────────────────────
    if (sp.add_open) {
        const int form_top = rows_top + static_cast<int>(stocks.size()) * kRowH;
        DrawLineEx(Vector2{static_cast<float>(bx), static_cast<float>(form_top)},
                   Vector2{static_cast<float>(bx + bw), static_cast<float>(form_top)}, 1.f, kLine);

        constexpr int kFrmPad    = 8;
        constexpr int kFldH      = 30;
        constexpr int kLblFont   = 13;
        static const char* add_labels[kStockAddFieldCount] = {"Symbol", "Name", "Price"};
        int fy = form_top + kFrmPad;

        for (int i = 0; i < kStockAddFieldCount; ++i) {
            DrawText(add_labels[i], bx + kFrmPad, fy, kLblFont, kMuted);
            fy += kLblFont + 3;
            draw_text_field(Rectangle{static_cast<float>(bx + kFrmPad), static_cast<float>(fy),
                                      static_cast<float>(bw - kFrmPad * 2), static_cast<float>(kFldH)},
                            add_labels[i], sp.add_fields[i], i == sp.add_active);
            fy += kFldH + kFrmPad;
        }

        // "Add Stock" button
        const int btn_w = bw - kFrmPad * 2;
        DrawRectangleRounded(Rectangle{static_cast<float>(bx + kFrmPad), static_cast<float>(fy),
                                       static_cast<float>(btn_w), static_cast<float>(kFldH)},
                             0.15f, 6, kBlue);
        const char* bl = "Add Stock";
        DrawText(bl, bx + kFrmPad + (btn_w - MeasureText(bl, 13)) / 2,
                 fy + (kFldH - 13) / 2, 13, kTitle);
    }

    EndScissorMode();

    // Error text at very bottom of panel (outside scissor)
    if (!sp.error.empty())
        DrawText(sp.error.c_str(), bx + 8, by + bh - 22, 13, kRed);
}

void PortfolioRenderer::draw_save_popup(const SavePopupState& popup,
                                         int screen_width, int screen_height) const {
    // Semi-transparent overlay
    DrawRectangle(0, 0, screen_width, screen_height, Color{0, 0, 0, 160});

    constexpr int kPopupW = 480;
    constexpr int kPopupH = 190;
    const int px = (screen_width  - kPopupW) / 2;
    const int py = (screen_height - kPopupH) / 2;

    DrawRectangleRounded(Rectangle{static_cast<float>(px + 4), static_cast<float>(py + 6),
                                   kPopupW, kPopupH}, 0.08f, 8, kShadow);
    DrawRectangleRounded(Rectangle{static_cast<float>(px), static_cast<float>(py),
                                   kPopupW, kPopupH}, 0.08f, 8, kPanelDark);
    DrawRectangleRoundedLines(Rectangle{static_cast<float>(px), static_cast<float>(py),
                                        kPopupW, kPopupH}, 0.08f, 8, kLine);

    DrawText("Save Portfolio As", px + 24, py + 22, 22, kTitle);
    DrawLineEx(Vector2{static_cast<float>(px + 24), static_cast<float>(py + 52)},
               Vector2{static_cast<float>(px + kPopupW - 24), static_cast<float>(py + 52)},
               1.0f, kLine);

    // Filename input field
    DrawText("Filename", px + 24, py + 64, 16, kMuted);
    const Rectangle field{static_cast<float>(px + 24), static_cast<float>(py + 84), kPopupW - 48, 40};
    draw_text_field(field, "", popup.filename, true);

    // Error
    if (!popup.error.empty())
        DrawText(popup.error.c_str(), px + 24, py + 132, 15, kRed);

    // Buttons
    constexpr int kBtnW = 100;
    constexpr int kBtnH = 36;
    const int confirm_x = px + kPopupW - 24 - kBtnW;
    const int cancel_x  = confirm_x - kBtnW - 12;
    const int btn_y = py + kPopupH - 24 - kBtnH;

    DrawRectangleRounded(Rectangle{static_cast<float>(cancel_x), static_cast<float>(btn_y),
                                   kBtnW, kBtnH}, 0.2f, 8, kTableHeader);
    DrawRectangleRoundedLines(Rectangle{static_cast<float>(cancel_x), static_cast<float>(btn_y),
                                        kBtnW, kBtnH}, 0.2f, 8, kLine);
    const char* cancel_lbl = "Cancel";
    DrawText(cancel_lbl, cancel_x + (kBtnW - MeasureText(cancel_lbl, 17)) / 2,
             btn_y + (kBtnH - 17) / 2, 17, kMuted);

    DrawRectangleRounded(Rectangle{static_cast<float>(confirm_x), static_cast<float>(btn_y),
                                   kBtnW, kBtnH}, 0.2f, 8, kBlue);
    const char* save_lbl = "Save";
    DrawText(save_lbl, confirm_x + (kBtnW - MeasureText(save_lbl, 17)) / 2,
             btn_y + (kBtnH - 17) / 2, 17, kTitle);
}

void PortfolioRenderer::draw_text_fit(const std::string& text, int x, int y, int max_width, int font_size, Color color) const {
    std::string visible = text;

    while (!visible.empty() && MeasureText(visible.c_str(), font_size) > max_width) {
        visible.pop_back();
    }

    if (visible.size() < text.size() && visible.size() > 3) {
        visible.resize(visible.size() - 3);
        visible += "...";
    }

    DrawText(visible.c_str(), x, y, font_size, color);
}

std::string PortfolioRenderer::money(double value) const {
    std::ostringstream out;
    out << "EUR " << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string PortfolioRenderer::number(double value) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string PortfolioRenderer::percent(double value) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value << "%";
    return out.str();
}

Color PortfolioRenderer::gain_color(double value) const {
    return value >= 0.0 ? kGreen : kRed;
}
