#ifndef GUI_PANEL_H
#define GUI_PANEL_H

#include "gui/element.h"
#include "gui/theme.h"

namespace gui {

struct PanelStyle {
    Color background = kSurface;
    Color border = kBorder;
    Color shadow = kShadow;
    float roundness = 0.08f;
    int segments = 8;
    bool draw_shadow = true;
};

class Panel : public Element {
public:
    Panel() = default;
    explicit Panel(Rectangle bounds);
    Panel(Rectangle bounds, PanelStyle style);

    void draw() const override;

    PanelStyle& style();
    const PanelStyle& style() const;

private:
    PanelStyle style_ = {};
};

}  // namespace gui

#endif
