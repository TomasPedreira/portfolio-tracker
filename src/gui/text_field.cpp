#include "gui/text_field.h"

#include <algorithm>
#include <utility>

namespace gui {

TextField::TextField(Rectangle bounds, std::string text)
    : Element(bounds), text_(std::move(text)) {}

void TextField::draw() const {
    DrawRectangleRounded(bounds_, style_.roundness, style_.segments, style_.background);
    DrawRectangleRoundedLines(bounds_, style_.roundness, style_.segments,
                              active_ ? style_.active_border : style_.border);

    if (active_) {
        DrawRectangleRoundedLines({bounds_.x - 1, bounds_.y - 1,
                                  bounds_.width + 2, bounds_.height + 2},
                                  style_.roundness, style_.segments, style_.active_border);
    }

    const int font_size = std::clamp((int)bounds_.height * 45 / 100, 14, 22);
    const int padding_x = std::max(8, (int)bounds_.width * 3 / 100);
    const int text_x = (int)bounds_.x + padding_x;
    const int text_y = (int)bounds_.y + ((int)bounds_.height - font_size) / 2;

    DrawText(text_.c_str(), text_x, text_y, font_size, style_.text);

    if (active_ && (int)(GetTime() * 2) % 2 == 0) {
        DrawRectangle(text_x + MeasureText(text_.c_str(), font_size) + 1, text_y,
                      2, font_size, style_.active_border);
    }
}

const std::string& TextField::text() const {
    return text_;
}

void TextField::set_text(std::string text) {
    text_ = std::move(text);
}

bool TextField::active() const {
    return active_;
}

void TextField::set_active(bool active) {
    active_ = active;
}

TextFieldStyle& TextField::style() {
    return style_;
}

const TextFieldStyle& TextField::style() const {
    return style_;
}

}  // namespace gui
