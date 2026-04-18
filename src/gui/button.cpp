#include "gui/button.h"

#include <utility>

namespace gui {

Button::Button(Rectangle bounds, std::string label, int font_size)
    : Element(bounds), label_(std::move(label)), font_size_(font_size) {}

void Button::draw() const {
    ButtonStyle effective_style = style_;
    if (disabled_) {
        effective_style.background = kSurfaceAlt;
        effective_style.border = kBorder;
        effective_style.text = kTextMuted;
    }

    DrawRectangleRounded(bounds_, effective_style.roundness, effective_style.segments,
                         effective_style.background);
    DrawRectangleRoundedLines(bounds_, effective_style.roundness, effective_style.segments,
                              effective_style.border);

    const int text_width = MeasureText(label_.c_str(), font_size_);
    const int text_x = (int)bounds_.x + ((int)bounds_.width - text_width) / 2;
    const int text_y = (int)bounds_.y + ((int)bounds_.height - font_size_) / 2;
    DrawText(label_.c_str(), text_x, text_y, font_size_, effective_style.text);
}

bool Button::contains(Vector2 point) const {
    return !disabled_ && Element::contains(point);
}

const std::string& Button::label() const {
    return label_;
}

void Button::set_label(std::string label) {
    label_ = std::move(label);
}

int Button::font_size() const {
    return font_size_;
}

void Button::set_font_size(int font_size) {
    font_size_ = font_size;
}

bool Button::disabled() const {
    return disabled_;
}

void Button::set_disabled(bool disabled) {
    disabled_ = disabled;
}

ButtonStyle& Button::style() {
    return style_;
}

const ButtonStyle& Button::style() const {
    return style_;
}

}  // namespace gui
