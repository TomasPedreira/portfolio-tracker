#ifndef GUI_LABEL_H
#define GUI_LABEL_H

#include "gui/element.h"
#include "gui/theme.h"

#include <string>

namespace gui {

struct LabelStyle {
    Color text = kText;
};

class Label : public Element {
public:
    Label() = default;
    Label(Rectangle bounds, std::string text, int font_size = 16);

    void draw() const override;

    const std::string& text() const;
    void set_text(std::string text);
    int font_size() const;
    void set_font_size(int font_size);
    LabelStyle& style();
    const LabelStyle& style() const;

private:
    std::string text_;
    int font_size_ = 16;
    LabelStyle style_ = {};
};

}  // namespace gui

#endif
