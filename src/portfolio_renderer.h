#ifndef PORTFOLIO_RENDERER_H
#define PORTFOLIO_RENDERER_H

#include "portfolio.h"
#include "raylib.h"

#include <string>

class PortfolioRenderer {
public:
    void draw(const Portfolio& portfolio, float scroll_offset) const;
    float max_scroll_offset(const Portfolio& portfolio, int screen_height) const;

private:
    void draw_header(const Portfolio& portfolio, int screen_width) const;
    void draw_summary_card(Rectangle bounds, const char* label, const std::string& value, Color accent) const;
    void draw_positions_table(const Portfolio& portfolio, Rectangle bounds, float scroll_offset) const;
    void draw_text_fit(const std::string& text, int x, int y, int max_width, int font_size, Color color) const;

    std::string money(double value) const;
    std::string number(double value) const;
    std::string percent(double value) const;
    Color gain_color(double value) const;
};

#endif
