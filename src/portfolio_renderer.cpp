#include "portfolio_renderer.h"

#include "gui/utils.h"
#include "position.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace {
constexpr int kMargin       = 32;
constexpr int kHeaderHeight = 126;
constexpr int kSummaryHeight = 110;
constexpr int kTableTopGap  = 28;
constexpr int kPanelTitleH  = 38;
constexpr int kPanelColHdrH = 34;
constexpr int kStockRowH    = 38;
constexpr int kPosRowH      = 32;
constexpr int kAddStockFormH = 124;  // height of the fixed add-stock form area

constexpr Color kTitle      = {232, 240, 248, 255};
constexpr Color kText       = {220, 232, 242, 255};
constexpr Color kMuted      = {151, 171, 190, 255};
constexpr Color kLine       = {71,  98,  122, 255};
constexpr Color kPanel      = {24,  52,  78,  255};
constexpr Color kPanelAlt   = {30,  61,  90,  255};
constexpr Color kTableHeader = {19, 43,  68,  255};
constexpr Color kGreen      = {74,  197, 139, 255};
constexpr Color kRed        = {238, 103, 112, 255};
constexpr Color kBlue       = {89,  153, 224, 255};
constexpr Color kInk        = {137, 178, 222, 255};
constexpr Color kShadow     = {5,   15,  25,  95};
constexpr Color kPanelDark  = {14,  34,  56,  255};

// Form layout helpers — proportional to screen dimensions.
struct FormMetrics {
    int pad_x, pad_y, col_width, col_gap;
    int label_font, label_gap, field_h, row_gap, btn_w;
    int col_x[3];
    int row_y[2];
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
    m.col_x[0]  = m.pad_x;
    m.col_x[1]  = m.pad_x + m.col_width + m.col_gap;
    m.col_x[2]  = m.pad_x + (m.col_width + m.col_gap) * 2;
    m.row_y[0]  = form_top + m.pad_y;
    m.row_y[1]  = m.row_y[0] + m.label_font + m.label_gap + m.field_h + m.row_gap;
    return m;
}

// Column x positions for collapsed stock rows and their header.
struct StockCols {
    int bx, bw;
    int arrow_x, sym_x, name_x, cur_x, amt_x, val_x, gain_x, del_x;
};

StockCols stock_cols(int bx, int bw) {
    StockCols c{};
    c.bx      = bx;
    c.bw      = bw;
    c.arrow_x = bx + 10;
    c.sym_x   = bx + 40;
    c.name_x  = bx + bw * 16 / 100;
    c.cur_x   = bx + bw * 44 / 100;
    c.amt_x   = bx + bw * 58 / 100;
    c.val_x   = bx + bw * 71 / 100;
    c.gain_x  = bx + bw * 83 / 100;
    c.del_x   = bx + bw - 34;
    return c;
}

// Column x positions for expanded position sub-rows and their sub-header.
struct PosCols {
    int dash_x, date_x, amt_x, buy_x, val_x, gain_x, del_x;
};

PosCols pos_cols(int bx, int bw) {
    PosCols c{};
    c.dash_x = bx + 10;
    c.date_x = bx + bw * 14 / 100;
    c.amt_x  = bx + bw * 34 / 100;
    c.buy_x  = bx + bw * 48 / 100;
    c.val_x  = bx + bw * 62 / 100;
    c.gain_x = bx + bw * 76 / 100;
    c.del_x  = bx + bw - 34;
    return c;
}
}  // namespace

// ── Public draw ──────────────────────────────────────────────────────────────

void PortfolioRenderer::draw(const Portfolio& portfolio, float scroll_offset,
                             const AddPanelState& panel, const SavePopupState& save_popup,
                             const PortfolioPanelState& panel_state) const {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    draw_header(portfolio, sw, save_popup);

    const int summary_top = kHeaderHeight;
    const int card_w = (sw - kMargin * 2 - 36) / 3;
    draw_summary_card({(float)kMargin, (float)summary_top, (float)card_w, (float)kSummaryHeight},
                      "Invested", money(portfolio.total_cost_basis()), kBlue);
    draw_summary_card({(float)(kMargin + card_w + 18), (float)summary_top, (float)card_w, (float)kSummaryHeight},
                      "Current value", money(portfolio.total_current_value()), kInk);
    draw_summary_card({(float)(kMargin + (card_w + 18) * 2), (float)summary_top, (float)card_w, (float)kSummaryHeight},
                      "Gain / loss",
                      money(portfolio.total_gain_loss()) + "  " + percent(portfolio.total_gain_loss_percent()),
                      gain_color(portfolio.total_gain_loss()));

    const int reserved  = panel_reserved(panel, sh);
    const int table_top = summary_top + kSummaryHeight + kTableTopGap;
    const int avail_h   = sh - table_top - kMargin - reserved;

    draw_portfolio_panel(portfolio,
                         {(float)kMargin, (float)table_top, (float)(sw - kMargin * 2), (float)avail_h},
                         scroll_offset, panel_state);

    draw_input_panel(panel, sw, sh);

    if (save_popup.open)
        draw_save_popup(save_popup, sw, sh);
}

float PortfolioRenderer::max_scroll_offset(const Portfolio& portfolio, int screen_height,
                                           const AddPanelState& panel,
                                           const PortfolioPanelState& panel_state) const {
    const int table_top   = kHeaderHeight + kSummaryHeight + kTableTopGap;
    const int panel_h     = screen_height - table_top - kMargin - panel_reserved(panel, screen_height);
    const int form_h      = panel_state.add_open ? kAddStockFormH : 0;
    const int list_visible = panel_h - kPanelTitleH - kPanelColHdrH - form_h;

    int content_h = 0;
    for (const Stock& st : portfolio.stocks()) {
        content_h += kStockRowH;
        if (panel_state.expanded.count(st.symbol())) {
            content_h += kPanelColHdrH; // position sub-header
            content_h += kPosRowH * (int)portfolio.position_indices_for(st.symbol()).size();
        }
    }
    return (float)std::max(0, content_h - list_visible);
}

// ── Header ────────────────────────────────────────────────────────────────────

void PortfolioRenderer::draw_header(const Portfolio& portfolio, int screen_width,
                                    const SavePopupState& save_popup) const {
    DrawText("Portfolio Tracker", kMargin, 24, 40, kTitle);

    const std::string subtitle = std::to_string(portfolio.positions().size()) + " open positions";
    DrawText(subtitle.c_str(), kMargin, 66, 22, kMuted);

    DrawLineEx({(float)kMargin, 88.f}, {(float)(screen_width - kMargin), 88.f}, 2.f, kLine);

    constexpr int kBtnW = 90, kBtnH = 36, btn_y = 26;
    const int btn_x = screen_width - kMargin - kBtnW;
    DrawRectangleRounded({(float)btn_x, (float)btn_y, kBtnW, kBtnH}, 0.2f, 8,
                         save_popup.file_set ? kBlue : kInk);
    const char* lbl = save_popup.file_set ? "Save" : "Save as";
    DrawText(lbl, btn_x + (kBtnW - MeasureText(lbl, 18)) / 2, btn_y + (kBtnH - 18) / 2, 18, kTitle);
}

// ── Summary card ─────────────────────────────────────────────────────────────

void PortfolioRenderer::draw_summary_card(Rectangle bounds, const char* label,
                                          const std::string& value, Color accent) const {
    DrawRectangleRounded({bounds.x + 4.f, bounds.y + 6.f, bounds.width, bounds.height}, 0.08f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.08f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.08f, 8, kLine);
    DrawRectangle((int)bounds.x, (int)bounds.y, 5, (int)bounds.height, accent);
    DrawText(label, (int)bounds.x + 22, (int)bounds.y + 22, 20, kMuted);
    draw_text_fit(value, (int)bounds.x + 22, (int)bounds.y + 58, (int)bounds.width - 44, 30, kTitle);
}

// ── Unified portfolio panel ───────────────────────────────────────────────────

void PortfolioRenderer::draw_portfolio_panel(const Portfolio& portfolio, Rectangle bounds,
                                             float scroll_offset,
                                             const PortfolioPanelState& s) const {
    const int bx = (int)bounds.x;
    const int by = (int)bounds.y;
    const int bw = (int)bounds.width;
    const int bh = (int)bounds.height;

    DrawRectangleRounded({bounds.x + 4.f, bounds.y + 6.f, bounds.width, bounds.height}, 0.04f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.04f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.04f, 8, kLine);

    // ── Title bar ────────────────────────────────────────────────────────────
    DrawRectangle(bx, by, bw, kPanelTitleH, kTableHeader);
    const char* title = "Portfolio";
    DrawText(title, bx + 16, by + (kPanelTitleH - 18) / 2, 18, kTitle);
    const int plus_x = bx + bw - 30;
    DrawText("+", plus_x, by + (kPanelTitleH - 22) / 2, 22, s.add_open ? kBlue : kInk);
    DrawLineEx({(float)bx, (float)(by + kPanelTitleH)},
               {(float)(bx + bw), (float)(by + kPanelTitleH)}, 1.f, kLine);

    // ── Add-stock form (fixed, below title) ──────────────────────────────────
    int list_top = by + kPanelTitleH;
    if (s.add_open) {
        DrawRectangle(bx, list_top, bw, kAddStockFormH, kPanelDark);
        constexpr int kPad = 14, kFldH = 30, kLblFont = 13;
        const int col_gap = 12;
        const int col_w = (bw - kPad * 2 - col_gap * 2 - 110) / 3;
        const int f_x[3] = { bx + kPad, bx + kPad + col_w + col_gap,
                              bx + kPad + (col_w + col_gap) * 2 };
        const int lbl_y  = list_top + 16;
        const int fld_y  = lbl_y + kLblFont + 4;
        static const char* add_lbls[3] = {"Symbol", "Name", "Price"};
        for (int i = 0; i < 3; ++i) {
            DrawText(add_lbls[i], f_x[i], lbl_y, kLblFont, kMuted);
            draw_text_field({(float)f_x[i], (float)fld_y, (float)col_w, (float)kFldH},
                            add_lbls[i], s.add_fields[i], i == s.add_active);
        }
        const int btn_x = f_x[2] + col_w + col_gap;
        const int btn_w = bx + bw - kPad - btn_x;
        const int btn_y = fld_y;
        DrawRectangleRounded({(float)btn_x, (float)btn_y, (float)btn_w, (float)kFldH}, 0.2f, 6, kBlue);
        const char* bl = "Add Stock";
        DrawText(bl, btn_x + (btn_w - MeasureText(bl, 13)) / 2, btn_y + (kFldH - 13) / 2, 13, kTitle);

        if (!s.error.empty())
            DrawText(s.error.c_str(), bx + kPad, fld_y + kFldH + 8, 13, kRed);

        DrawLineEx({(float)bx, (float)(list_top + kAddStockFormH)},
                   {(float)(bx + bw), (float)(list_top + kAddStockFormH)}, 1.f, kLine);
        list_top += kAddStockFormH;
    }

    // ── Column header row ────────────────────────────────────────────────────
    const StockCols c = stock_cols(bx, bw);
    const PosCols   pc = pos_cols(bx, bw);
    DrawRectangle(bx, list_top, bw, kPanelColHdrH, kTableHeader);
    DrawText("Symbol",  c.sym_x,  list_top + 10, 15, kText);
    DrawText("Name",    c.name_x, list_top + 10, 15, kText);
    DrawText("Current", c.cur_x,  list_top + 10, 15, kText);
    DrawText("Shares",  c.amt_x,  list_top + 10, 15, kText);
    DrawText("Value",   c.val_x,  list_top + 10, 15, kText);
    DrawText("Gain",    c.gain_x, list_top + 10, 15, kText);
    DrawLineEx({(float)bx, (float)(list_top + kPanelColHdrH)},
               {(float)(bx + bw), (float)(list_top + kPanelColHdrH)}, 1.f, kLine);
    list_top += kPanelColHdrH;

    // ── Scrollable list ───────────────────────────────────────────────────────
    const int scroll_h = bh - (list_top - by);
    BeginScissorMode(bx, list_top, bw, scroll_h);

    const Vector2 mouse = GetMousePosition();
    const auto& stocks  = portfolio.stocks();
    int y = list_top - (int)scroll_offset;

    for (int i = 0; i < (int)stocks.size(); ++i) {
        const Stock& st = stocks[i];
        const bool expanded = s.expanded.count(st.symbol()) > 0;
        const bool sel      = (s.selected_stock == i);

        // Stock row
        DrawRectangle(bx, y, bw, kStockRowH, sel ? kPanelAlt : kPanel);

        // Expand arrow
        DrawText(expanded ? "v" : ">", c.arrow_x, y + (kStockRowH - 18) / 2, 18, kInk);

        DrawText(st.symbol().c_str(), c.sym_x, y + (kStockRowH - 17) / 2, 17, kTitle);
        draw_text_fit(st.name(), c.name_x, y + (kStockRowH - 15) / 2,
                      c.cur_x - c.name_x - 12, 15, kText);

        // Current price — editable on second click
        if (sel && s.editing_price) {
            draw_text_field({(float)(c.cur_x - 2), (float)(y + 5),
                             (float)(c.amt_x - c.cur_x - 10), (float)(kStockRowH - 10)},
                            "", s.price_buf, true);
        } else {
            DrawText(money(st.current_price()).c_str(), c.cur_x, y + (kStockRowH - 15) / 2, 15, kText);
        }

        // Aggregate totals across this stock's positions
        double total_shares = 0, total_value = 0, total_cost = 0;
        for (std::size_t idx : portfolio.position_indices_for(st.symbol())) {
            const Position& p = portfolio.positions()[idx];
            total_shares += p.amount();
            total_value  += p.current_value();
            total_cost   += p.cost_basis();
        }
        DrawText(number(total_shares).c_str(), c.amt_x, y + (kStockRowH - 15) / 2, 15, kText);
        DrawText(money(total_value).c_str(),   c.val_x, y + (kStockRowH - 15) / 2, 15, kText);
        const double gpct = total_cost > 0 ? (total_value - total_cost) / total_cost * 100.0 : 0.0;
        DrawText(percent(gpct).c_str(), c.gain_x, y + (kStockRowH - 15) / 2, 15,
                 gain_color(total_value - total_cost));

        // Delete on hover
        const bool hov = mouse.x >= bx && mouse.x <= bx + bw &&
                         mouse.y >= y  && mouse.y <  y + kStockRowH;
        if (hov) {
            DrawRectangleRounded({(float)c.del_x, (float)(y + (kStockRowH - 22) / 2), 22.f, 22.f},
                                 0.25f, 6, {161, 61, 70, 255});
            DrawText("x", c.del_x + 6, y + (kStockRowH - 22) / 2 + 2, 18, kTitle);
        }
        DrawLine(bx, y + kStockRowH, bx + bw, y + kStockRowH, kLine);
        y += kStockRowH;

        // Position sub-rows
        if (expanded) {
            // Sub-header for position columns
            DrawRectangle(bx, y, bw, kPanelColHdrH, kPanelAlt);
            DrawText("Date",    pc.date_x, y + (kPanelColHdrH - 13) / 2, 13, kText);
            DrawText("Shares",  pc.amt_x,  y + (kPanelColHdrH - 13) / 2, 13, kText);
            DrawText("Buy",     pc.buy_x,  y + (kPanelColHdrH - 13) / 2, 13, kText);
            DrawText("Value",   pc.val_x,  y + (kPanelColHdrH - 13) / 2, 13, kText);
            DrawText("Gain",    pc.gain_x, y + (kPanelColHdrH - 13) / 2, 13, kText);
            DrawText("+", bx + bw - 54, y + (kPanelColHdrH - 18) / 2, 18, kInk);
            DrawLine(bx, y + kPanelColHdrH, bx + bw, y + kPanelColHdrH, kLine);
            y += kPanelColHdrH;

            const auto& positions = portfolio.positions();
            for (std::size_t idx : portfolio.position_indices_for(st.symbol())) {
                const Position& p = positions[idx];
                const bool phov = mouse.x >= bx && mouse.x <= bx + bw &&
                                  mouse.y >= y  && mouse.y <  y + kPosRowH;
                const bool editing_this = (s.editing_stock_idx == i) &&
                                          (s.editing_position_idx == (int)idx);

                DrawRectangle(bx, y, bw, kPosRowH, kPanelDark);

                // Date
                if (editing_this && s.editing_field == PositionEditField::Date) {
                    draw_text_field({(float)(pc.date_x - 2), (float)(y + 2),
                                     (float)(pc.amt_x - pc.date_x - 8), (float)(kPosRowH - 4)},
                                    "", s.edit_buf, true);
                } else {
                    DrawText(p.acquisition_date().c_str(), pc.date_x, y + (kPosRowH - 14) / 2, 14, kText);
                }

                // Amount
                if (editing_this && s.editing_field == PositionEditField::Amount) {
                    draw_text_field({(float)(pc.amt_x - 2), (float)(y + 2),
                                     (float)(pc.buy_x - pc.amt_x - 8), (float)(kPosRowH - 4)},
                                    "", s.edit_buf, true);
                } else {
                    DrawText(number(p.amount()).c_str(), pc.amt_x, y + (kPosRowH - 14) / 2, 14, kText);
                }

                // Buy price
                if (editing_this && s.editing_field == PositionEditField::BuyPrice) {
                    draw_text_field({(float)(pc.buy_x - 2), (float)(y + 2),
                                     (float)(pc.val_x - pc.buy_x - 8), (float)(kPosRowH - 4)},
                                    "", s.edit_buf, true);
                } else {
                    DrawText(money(p.buy_price()).c_str(), pc.buy_x, y + (kPosRowH - 14) / 2, 14, kText);
                }

                DrawText(money(p.current_value()).c_str(), pc.val_x, y + (kPosRowH - 14) / 2, 14, kText);
                DrawText(percent(p.gain_loss_percent()).c_str(), pc.gain_x, y + (kPosRowH - 14) / 2, 14,
                         gain_color(p.gain_loss()));

                // Delete on hover
                if (phov) {
                    DrawRectangleRounded({(float)pc.del_x, (float)(y + (kPosRowH - 20) / 2), 20.f, 20.f},
                                         0.25f, 6, {161, 61, 70, 255});
                    DrawText("x", pc.del_x + 5, y + (kPosRowH - 20) / 2 + 2, 16, kTitle);
                }
                DrawLine(bx, y + kPosRowH, bx + bw, y + kPosRowH, kLine);
                y += kPosRowH;
            }
        }
    }

    EndScissorMode();

    // Inline-edit error shown at bottom of panel (outside scissor)
    if (!s.error.empty() && !s.add_open) {
        DrawText(s.error.c_str(), bx + 12, by + bh - 22, 13, kRed);
    }
}

// ── Add-position bottom tab ───────────────────────────────────────────────────

void PortfolioRenderer::draw_input_panel(const AddPanelState& panel, int screen_width, int screen_height) const {
    const int tab_h    = panel_tab_height(screen_height);
    const int reserved = panel_reserved(panel, screen_height);
    const int panel_top = screen_height - reserved;

    DrawRectangle(0, panel_top, screen_width, reserved, kPanelDark);
    DrawLineEx({0.f, (float)panel_top}, {(float)screen_width, (float)panel_top}, 2.f, kLine);

    const int tab_font  = std::clamp(tab_h * 45 / 100, 14, 22);
    const int tab_text_y = panel_top + (tab_h - tab_font) / 2;
    DrawText("+ Add Position", kMargin, tab_text_y, tab_font, panel.open ? kBlue : kInk);

    if (!panel.open) {
        const char* hint = "N  to open  |  scroll: wheel / Up / Down  |  Home: top";
        const int hw = MeasureText(hint, tab_font - 2);
        DrawText(hint, screen_width - kMargin - hw, tab_text_y + 1, tab_font - 2, kMuted);
        return;
    }

    const char* close_hint = "Esc to close";
    const int cw = MeasureText(close_hint, tab_font - 2);
    DrawText(close_hint, screen_width - kMargin - cw, tab_text_y + 1, tab_font - 2, kMuted);

    const int form_h  = panel_form_height(screen_height);
    const int form_top = panel_top + tab_h;
    const FormMetrics m = compute_form(screen_width, screen_height, form_top, form_h);

    static const char* labels[kFieldCount] = {"Symbol", "Buy Price", "Amount", "Date (YYYY-MM-DD)"};

    for (int i = 0; i < kFieldCount; ++i) {
        const int col = i % 3, row = i / 3;
        const int lx  = m.col_x[col], ly = m.row_y[row];
        DrawText(labels[i], lx, ly, m.label_font, kText);
        draw_text_field({(float)lx, (float)(ly + m.label_font + m.label_gap),
                         (float)m.col_width, (float)m.field_h},
                        labels[i], panel.fields[i], i == panel.active_field);
    }

    const int bottom_y = m.row_y[1] + m.label_font + m.label_gap + m.field_h + std::max(4, m.label_gap);
    const int btn_h    = std::clamp(m.field_h * 80 / 100, 28, 42);
    const Rectangle btn_rect{(float)(screen_width - kMargin - m.btn_w), (float)bottom_y,
                              (float)m.btn_w, (float)btn_h};
    DrawRectangleRounded(btn_rect, 0.2f, 8, kBlue);
    const char* btn_lbl = "Add";
    const int btn_font  = std::clamp(btn_h * 50 / 100, 13, 20);
    DrawText(btn_lbl,
             (int)btn_rect.x + (m.btn_w - MeasureText(btn_lbl, btn_font)) / 2,
             (int)btn_rect.y + (btn_h - btn_font) / 2, btn_font, kTitle);

    const int msg_y = bottom_y + (btn_h - m.label_font) / 2;
    if (!panel.error_message.empty())
        DrawText(panel.error_message.c_str(), kMargin, msg_y, m.label_font, kRed);
    else
        DrawText("Tab / click to switch fields  |  Enter to add", kMargin, msg_y, m.label_font - 1, kMuted);
}

// ── Text field ────────────────────────────────────────────────────────────────

void PortfolioRenderer::draw_text_field(Rectangle bounds, const char* /*label*/, const char* text, bool active) const {
    DrawRectangleRounded(bounds, 0.12f, 6, kPanel);
    DrawRectangleRoundedLines(bounds, 0.12f, 6, active ? kBlue : kLine);
    if (active)
        DrawRectangleRoundedLines({bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2}, 0.12f, 6, kBlue);

    const int fs   = std::clamp((int)bounds.height * 45 / 100, 14, 22);
    const int padx = std::max(8, (int)bounds.width * 3 / 100);
    const int tx   = (int)bounds.x + padx;
    const int ty   = (int)bounds.y + ((int)bounds.height - fs) / 2;

    DrawText(text, tx, ty, fs, kTitle);
    if (active && (int)(GetTime() * 2) % 2 == 0)
        DrawRectangle(tx + MeasureText(text, fs) + 1, ty, 2, fs, kBlue);
}

// ── Save popup ────────────────────────────────────────────────────────────────

void PortfolioRenderer::draw_save_popup(const SavePopupState& popup, int sw, int sh) const {
    DrawRectangle(0, 0, sw, sh, {0, 0, 0, 160});

    constexpr int kW = 480, kH = 190;
    const int px = (sw - kW) / 2, py = (sh - kH) / 2;

    DrawRectangleRounded({(float)(px+4), (float)(py+6), kW, kH}, 0.08f, 8, kShadow);
    DrawRectangleRounded({(float)px, (float)py, kW, kH}, 0.08f, 8, kPanelDark);
    DrawRectangleRoundedLines({(float)px, (float)py, kW, kH}, 0.08f, 8, kLine);

    DrawText("Save Portfolio As", px + 24, py + 22, 22, kTitle);
    DrawLineEx({(float)(px+24), (float)(py+52)}, {(float)(px+kW-24), (float)(py+52)}, 1.f, kLine);

    DrawText("Filename", px + 24, py + 64, 16, kMuted);
    draw_text_field({(float)(px+24), (float)(py+84), (float)(kW-48), 40}, "", popup.filename, true);

    if (!popup.error.empty())
        DrawText(popup.error.c_str(), px + 24, py + 132, 15, kRed);

    constexpr int kBtnW = 100, kBtnH = 36;
    const int cx = px + kW - 24 - kBtnW;
    const int canx = cx - kBtnW - 12;
    const int btn_y = py + kH - 24 - kBtnH;

    DrawRectangleRounded({(float)canx, (float)btn_y, kBtnW, kBtnH}, 0.2f, 8, kTableHeader);
    DrawRectangleRoundedLines({(float)canx, (float)btn_y, kBtnW, kBtnH}, 0.2f, 8, kLine);
    const char* cl = "Cancel";
    DrawText(cl, canx + (kBtnW - MeasureText(cl, 17)) / 2, btn_y + (kBtnH - 17) / 2, 17, kMuted);

    DrawRectangleRounded({(float)cx, (float)btn_y, kBtnW, kBtnH}, 0.2f, 8, kBlue);
    const char* sl = "Save";
    DrawText(sl, cx + (kBtnW - MeasureText(sl, 17)) / 2, btn_y + (kBtnH - 17) / 2, 17, kTitle);
}

// ── Text fit ──────────────────────────────────────────────────────────────────

void PortfolioRenderer::draw_text_fit(const std::string& text, int x, int y, int max_width, int font_size, Color color) const {
    std::string v = text;
    while (!v.empty() && MeasureText(v.c_str(), font_size) > max_width) v.pop_back();
    if (v.size() < text.size() && v.size() > 3) { v.resize(v.size() - 3); v += "..."; }
    DrawText(v.c_str(), x, y, font_size, color);
}

// ── Formatters ────────────────────────────────────────────────────────────────

std::string PortfolioRenderer::money(double value) const {
    std::ostringstream o; o << "EUR " << std::fixed << std::setprecision(2) << value; return o.str();
}
std::string PortfolioRenderer::number(double value) const {
    std::ostringstream o; o << std::fixed << std::setprecision(2) << value; return o.str();
}
std::string PortfolioRenderer::percent(double value) const {
    std::ostringstream o; o << std::fixed << std::setprecision(2) << value << "%"; return o.str();
}
Color PortfolioRenderer::gain_color(double value) const {
    return value >= 0.0 ? kGreen : kRed;
}
