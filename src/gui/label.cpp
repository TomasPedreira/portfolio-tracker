#include "gui/label.h"

#include "gui/utils.h"

#include <utility>

namespace gui {

Label::Label(Rectangle bounds, std::string text, int font_size)
    : Element(bounds), text_(std::move(text)), font_size_(font_size) {}

void Label::draw() const {
    if (bounds_.width > 0) {
        draw_text_fit(text_, (int)bounds_.x, (int)bounds_.y,
                      (int)bounds_.width, font_size_, style_.text);
        return;
    }

    DrawText(text_.c_str(), (int)bounds_.x, (int)bounds_.y, font_size_, style_.text);
}

const std::string& Label::text() const {
    return text_;
}

void Label::set_text(std::string text) {
    text_ = std::move(text);
}

int Label::font_size() const {
    return font_size_;
}

void Label::set_font_size(int font_size) {
    font_size_ = font_size;
}

LabelStyle& Label::style() {
    return style_;
}

const LabelStyle& Label::style() const {
    return style_;
}

}  // namespace gui
