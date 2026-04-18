#ifndef GUI_ELEMENT_H
#define GUI_ELEMENT_H

#include "raylib.h"

namespace gui {

class Element {
public:
    Element() = default;
    explicit Element(Rectangle bounds);
    virtual ~Element() = default;

    virtual void draw() const = 0;
    virtual bool contains(Vector2 point) const;

    Rectangle bounds() const;
    void set_bounds(Rectangle bounds);

protected:
    Rectangle bounds_ = {};
};

}  // namespace gui

#endif
