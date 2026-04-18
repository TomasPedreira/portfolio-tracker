#ifndef GUI_FORM_H
#define GUI_FORM_H

#include "gui/element.h"

namespace gui {

struct FieldGridMetrics {
    int pad_x = 0;
    int pad_y = 0;
    int column_width = 0;
    int column_gap = 0;
    int label_font = 0;
    int label_gap = 0;
    int field_height = 0;
    int row_gap = 0;
    int button_width = 0;
    int column_x[3] = {};
    int row_y[2] = {};
    int top = 0;
};

class Form : public Element {
public:
    Form() = default;
    explicit Form(Rectangle bounds);

    void draw() const override;

    int columns() const;
    void set_columns(int columns);
    int gap() const;
    void set_gap(int gap);
    int label_font() const;
    void set_label_font(int font_size);
    int label_gap() const;
    void set_label_gap(int gap);
    int field_height() const;
    void set_field_height(int height);
    int row_gap() const;
    void set_row_gap(int gap);

    Rectangle field_bounds(int index) const;
    Vector2 label_position(int index) const;

private:
    int columns_ = 1;
    int gap_ = 12;
    int label_font_ = 14;
    int label_gap_ = 4;
    int field_height_ = 36;
    int row_gap_ = 12;
};

FieldGridMetrics compute_field_grid(int screen_width, int top, int height);

}  // namespace gui

#endif
