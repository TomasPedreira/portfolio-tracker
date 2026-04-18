#include "gui/form.h"

#include <algorithm>

namespace gui {

Form::Form(Rectangle bounds) : Element(bounds) {}

void Form::draw() const {}

int Form::columns() const {
    return columns_;
}

void Form::set_columns(int columns) {
    columns_ = std::max(1, columns);
}

int Form::gap() const {
    return gap_;
}

void Form::set_gap(int gap) {
    gap_ = std::max(0, gap);
}

int Form::label_font() const {
    return label_font_;
}

void Form::set_label_font(int font_size) {
    label_font_ = std::max(1, font_size);
}

int Form::label_gap() const {
    return label_gap_;
}

void Form::set_label_gap(int gap) {
    label_gap_ = std::max(0, gap);
}

int Form::field_height() const {
    return field_height_;
}

void Form::set_field_height(int height) {
    field_height_ = std::max(1, height);
}

int Form::row_gap() const {
    return row_gap_;
}

void Form::set_row_gap(int gap) {
    row_gap_ = std::max(0, gap);
}

Rectangle Form::field_bounds(int index) const {
    const int safe_columns = std::max(1, columns_);
    const int column = index % safe_columns;
    const int row = index / safe_columns;
    const float available_width = bounds_.width - (float)gap_ * (safe_columns - 1);
    const float field_width = available_width / safe_columns;
    const float x = bounds_.x + column * (field_width + gap_);
    const float y = bounds_.y + row * (label_font_ + label_gap_ + field_height_ + row_gap_) +
                    label_font_ + label_gap_;
    return {x, y, field_width, (float)field_height_};
}

Vector2 Form::label_position(int index) const {
    const Rectangle field = field_bounds(index);
    return {field.x, field.y - label_gap_ - label_font_};
}

FieldGridMetrics compute_field_grid(int screen_width, int top, int height) {
    FieldGridMetrics m{};
    m.top = top;
    m.pad_x = 32;
    m.pad_y = std::clamp(height * 9 / 100, 10, 18);
    m.column_gap = 18;
    m.column_width = (screen_width - m.pad_x * 2 - m.column_gap * 2) / 3;
    m.label_font = std::clamp(height / 13, 13, 18);
    m.label_gap = std::max(3, height / 60);
    m.field_height = std::clamp(height * 22 / 100, 34, 50);
    m.row_gap = std::clamp(height * 8 / 100, 8, 18);
    m.button_width = std::clamp(screen_width / 12, 80, 120);
    m.column_x[0] = m.pad_x;
    m.column_x[1] = m.pad_x + m.column_width + m.column_gap;
    m.column_x[2] = m.pad_x + (m.column_width + m.column_gap) * 2;
    m.row_y[0] = top + m.pad_y;
    m.row_y[1] = m.row_y[0] + m.label_font + m.label_gap + m.field_height + m.row_gap;
    return m;
}

}  // namespace gui
