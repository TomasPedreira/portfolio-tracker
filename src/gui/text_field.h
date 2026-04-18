#ifndef GUI_TEXT_FIELD_H
#define GUI_TEXT_FIELD_H

#include "gui/element.h"
#include "gui/theme.h"

#include <string>

namespace gui {

struct TextFieldStyle {
    Color background = kSurface;
    Color border = kBorder;
    Color active_border = kAccent;
    Color text = kTextStrong;
    float roundness = 0.12f;
    int segments = 6;
};

class TextField : public Element {
public:
    TextField() = default;
    TextField(Rectangle bounds, std::string text = {});

    void draw() const override;

    const std::string& text() const;
    void set_text(std::string text);
    bool active() const;
    void set_active(bool active);
    TextFieldStyle& style();
    const TextFieldStyle& style() const;

private:
    std::string text_;
    bool active_ = false;
    TextFieldStyle style_ = {};
};

}  // namespace gui

#endif
