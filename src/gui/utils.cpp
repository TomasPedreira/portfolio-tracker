#include "utils.h"

namespace gui {

bool contains_point(Rectangle bounds, Vector2 point) {
    return point.x >= bounds.x && point.x <= bounds.x + bounds.width &&
           point.y >= bounds.y && point.y <= bounds.y + bounds.height;
}

void draw_text_fit(const std::string& text, int x, int y, int max_width,
                   int font_size, Color color) {
    std::string visible = text;
    while (!visible.empty() && MeasureText(visible.c_str(), font_size) > max_width)
        visible.pop_back();

    if (visible.size() < text.size() && visible.size() > 3) {
        visible.resize(visible.size() - 3);
        visible += "...";
    }

    DrawText(visible.c_str(), x, y, font_size, color);
}

void draw_text_bold(const char* text, int x, int y, int font_size, Color color) {
    Color darker = {
        color.r > 60 ? (unsigned char)(color.r - 60) : 0,
        color.g > 60 ? (unsigned char)(color.g - 60) : 0,
        color.b > 60 ? (unsigned char)(color.b - 60) : 0,
        color.a
    };
    DrawText(text, x + 1, y + 1, font_size, darker);
    DrawText(text, x, y, font_size, color);
}

}  // namespace gui
