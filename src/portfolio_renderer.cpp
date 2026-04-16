#include "portfolio_renderer.h"

#include "position.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
constexpr int kMargin = 32;
constexpr int kHeaderHeight = 126;
constexpr int kSummaryHeight = 110;
constexpr int kTableTopGap = 28;
constexpr int kTableHeaderHeight = 44;
constexpr int kRowHeight = 52;
constexpr int kFooterHintHeight = 34;

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

int table_content_height(const Portfolio& portfolio) {
    return kTableHeaderHeight + static_cast<int>(portfolio.positions().size()) * kRowHeight;
}
}

void PortfolioRenderer::draw(const Portfolio& portfolio, float scroll_offset) const {
    const int screen_width = GetScreenWidth();
    const int screen_height = GetScreenHeight();

    draw_header(portfolio, screen_width);

    const int summary_top = kHeaderHeight;
    const int card_width = (screen_width - kMargin * 2 - 36) / 3;
    draw_summary_card(Rectangle{static_cast<float>(kMargin), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Invested", money(portfolio.total_cost_basis()), kBlue);
    draw_summary_card(Rectangle{static_cast<float>(kMargin + card_width + 18), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Current value", money(portfolio.total_current_value()), kInk);
    draw_summary_card(Rectangle{static_cast<float>(kMargin + (card_width + 18) * 2), static_cast<float>(summary_top),
                                static_cast<float>(card_width), static_cast<float>(kSummaryHeight)},
                      "Gain / loss", money(portfolio.total_gain_loss()) + "  " + percent(portfolio.total_gain_loss_percent()),
                      gain_color(portfolio.total_gain_loss()));

    const float table_top = static_cast<float>(summary_top + kSummaryHeight + kTableTopGap);
    const Rectangle table_bounds{
        static_cast<float>(kMargin),
        table_top,
        static_cast<float>(screen_width - kMargin * 2),
        static_cast<float>(screen_height - static_cast<int>(table_top) - kMargin - kFooterHintHeight),
    };

    draw_positions_table(portfolio, table_bounds, scroll_offset);

    DrawText("Mouse wheel or Up/Down to scroll. Home returns to top.",
             kMargin, screen_height - kMargin + 4, 20, kMuted);
}

float PortfolioRenderer::max_scroll_offset(const Portfolio& portfolio, int screen_height) const {
    const int table_top = kHeaderHeight + kSummaryHeight + kTableTopGap;
    const int visible_height = screen_height - table_top - kMargin - kFooterHintHeight;
    const int overflow = table_content_height(portfolio) - visible_height;
    return static_cast<float>(std::max(0, overflow));
}

void PortfolioRenderer::draw_header(const Portfolio& portfolio, int screen_width) const {
    DrawText("Portfolio Tracker", kMargin, 24, 40, kTitle);

    const std::string subtitle = std::to_string(portfolio.positions().size()) + " open positions";
    DrawText(subtitle.c_str(), kMargin, 66, 22, kMuted);

    DrawLineEx(Vector2{static_cast<float>(kMargin), 88.0f},
               Vector2{static_cast<float>(screen_width - kMargin), 88.0f}, 2.0f, kLine);
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

void PortfolioRenderer::draw_positions_table(const Portfolio& portfolio, Rectangle bounds, float scroll_offset) const {
    DrawRectangleRounded(Rectangle{bounds.x + 5.0f, bounds.y + 7.0f, bounds.width, bounds.height},
                         0.04f, 8, kShadow);
    DrawRectangleRounded(bounds, 0.04f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.04f, 8, kLine);

    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y),
                     static_cast<int>(bounds.width), static_cast<int>(bounds.height));

    const int left = static_cast<int>(bounds.x);
    const int top = static_cast<int>(bounds.y) - static_cast<int>(scroll_offset);
    const int symbol_x = left + 20;
    const int name_x = left + 120;
    const int amount_x = left + 460;
    const int buy_x = left + 575;
    const int current_x = left + 710;
    const int value_x = left + 850;
    const int gain_x = left + 1000;
    const int date_x = left + 1120;

    DrawRectangle(left, top, static_cast<int>(bounds.width), kTableHeaderHeight, kTableHeader);
    DrawText("Symbol", symbol_x, top + 13, 20, kMuted);
    DrawText("Name", name_x, top + 13, 20, kMuted);
    DrawText("Amount", amount_x, top + 13, 20, kMuted);
    DrawText("Buy", buy_x, top + 13, 20, kMuted);
    DrawText("Current", current_x, top + 13, 20, kMuted);
    DrawText("Value", value_x, top + 13, 20, kMuted);
    DrawText("Gain", gain_x, top + 13, 20, kMuted);
    DrawText("Date", date_x, top + 13, 20, kMuted);
    DrawLineEx(Vector2{static_cast<float>(left), static_cast<float>(top + kTableHeaderHeight)},
               Vector2{static_cast<float>(left + static_cast<int>(bounds.width)), static_cast<float>(top + kTableHeaderHeight)},
               2.0f, kLine);

    int y = top + kTableHeaderHeight;
    for (const Position& position : portfolio.positions()) {
        const Color row_color = ((y / kRowHeight) % 2 == 0) ? kPanel : kPanelAlt;
        DrawRectangle(left, y, static_cast<int>(bounds.width), kRowHeight, row_color);
        DrawLine(left + 16, y + kRowHeight, left + static_cast<int>(bounds.width) - 16, y + kRowHeight, kLine);

        DrawText(position.stock().symbol().c_str(), symbol_x, y + 15, 21, kTitle);
        draw_text_fit(position.stock().name(), name_x, y + 15, amount_x - name_x - 18, 21, kText);
        DrawText(number(position.amount()).c_str(), amount_x, y + 15, 21, kText);
        DrawText(money(position.buy_price()).c_str(), buy_x, y + 15, 21, kText);
        DrawText(money(position.stock().current_price()).c_str(), current_x, y + 15, 21, kText);
        DrawText(money(position.current_value()).c_str(), value_x, y + 15, 21, kText);
        DrawText(percent(position.gain_loss_percent()).c_str(), gain_x, y + 15, 21, gain_color(position.gain_loss()));
        DrawText(position.acquisition_date().c_str(), date_x, y + 15, 21, kText);

        y += kRowHeight;
    }

    EndScissorMode();
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
