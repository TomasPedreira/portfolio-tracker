#include "gui/element.h"

#include "gui/utils.h"

namespace gui {

Element::Element(Rectangle bounds) : bounds_(bounds) {}

bool Element::contains(Vector2 point) const {
    return contains_point(bounds_, point);
}

Rectangle Element::bounds() const {
    return bounds_;
}

void Element::set_bounds(Rectangle bounds) {
    bounds_ = bounds;
}

}  // namespace gui
