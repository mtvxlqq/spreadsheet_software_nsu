#pragma once

#include "core/address.hpp"

namespace ss::app {

// Какая часть листа сейчас видна на экране. Ширина столбцов пока
// одинаковая, на втором этапе тут появятся свои ширины.
class Viewport {
public:
    int first_row() const {
        return first_row_;
    }

    int first_col() const {
        return first_col_;
    }

    int visible_rows() const {
        return visible_rows_;
    }

    int visible_cols() const {
        return visible_cols_;
    }

    // размер меньше 1 считаем за 1, чтобы не делить пустой экран
    void resize(int visible_rows, int visible_cols);
    // прокручивает ровно настолько, чтобы ячейка стала видна
    void ensure_visible(ss::core::CellAddress cell);
    bool contains(ss::core::CellAddress cell) const;

private:
    int first_row_ = 0;
    int first_col_ = 0;
    int visible_rows_ = 1;
    int visible_cols_ = 1;
};

}  // namespace ss::app
