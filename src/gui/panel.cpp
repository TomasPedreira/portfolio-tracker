#include "gui/panel.h"

namespace gui {

Panel::Panel(Rectangle bounds) : Element(bounds) {}

Panel::Panel(Rectangle bounds, PanelStyle style)
    : Element(bounds), style_(style) {}

void Panel::draw() const {
    if (style_.draw_shadow) {
        DrawRectangleRounded({bounds_.x + 4.f, bounds_.y + 6.f, bounds_.width, bounds_.height},
                             style_.roundness, style_.segments, style_.shadow);
    }

    DrawRectangleRounded(bounds_, style_.roundness, style_.segments, style_.background);
    DrawRectangleRoundedLines(bounds_, style_.roundness, style_.segments, style_.border);
}

PanelStyle& Panel::style() {
    return style_;
}

const PanelStyle& Panel::style() const {
    return style_;
}

}  // namespace gui
