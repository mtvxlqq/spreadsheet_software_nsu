#pragma once

#include "core/address.hpp"
#include "core/column_widths.hpp"

namespace ss::app {

// Какая часть листа сейчас видна на экране. Строки считаются штуками,
// а столбцы разной ширины, поэтому для них известна ширина окна в позициях
// экрана, а сколько столбцов в неё влезет, зависит от ColumnWidths.
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

    // ширина сетки в позициях экрана, без номеров строк
    int width() const {
        return width_;
    }

    // Сколько столбцов видно целиком, начиная с first_col. Хотя бы один:
    // столбец шире окна всё равно показываем, просто обрезанным.
    int visible_cols(const ss::core::ColumnWidths& widths) const;

    // размер меньше 1 считаем за 1, чтобы не делить пустой экран
    void resize(int visible_rows, int width);
    // прокручивает ровно настолько, чтобы ячейка стала видна целиком
    void ensure_visible(ss::core::CellAddress cell, const ss::core::ColumnWidths& widths);
    bool contains(ss::core::CellAddress cell, const ss::core::ColumnWidths& widths) const;

private:
    // самый левый столбец, с которого last ещё виден целиком
    int leftmost_for(int last, const ss::core::ColumnWidths& widths) const;

    int first_row_ = 0;
    int first_col_ = 0;
    int visible_rows_ = 1;
    int width_ = 1;
};

}  // namespace ss::app
