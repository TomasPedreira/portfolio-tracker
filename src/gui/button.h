#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "gui/element.h"
#include "gui/theme.h"

#include <string>

namespace gui {

struct ButtonStyle {
    Color background = kAccent;
    Color border = kAccent;
    Color text = kTextStrong;
    float roundness = 0.2f;
    int segments = 8;
};

class Button : public Element {
public:
    Button() = default;
    Button(Rectangle bounds, std::string label, int font_size = 16);

    void draw() const override;
    bool contains(Vector2 point) const override;

    const std::string& label() const;
    void set_label(std::string label);
    int font_size() const;
    void set_font_size(int font_size);
    bool disabled() const;
    void set_disabled(bool disabled);
    ButtonStyle& style();
    const ButtonStyle& style() const;

private:
    std::string label_;
    int font_size_ = 16;
    bool disabled_ = false;
    ButtonStyle style_ = {};
};

}  // namespace gui

#endif
