#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "raylib.h"

#include <string>

namespace gui {

bool contains_point(Rectangle bounds, Vector2 point);
void draw_text_fit(const std::string& text, int x, int y, int max_width,
                   int font_size, Color color);
void draw_text_bold(const char* text, int x, int y, int font_size, Color color);

}  // namespace gui

#endif
