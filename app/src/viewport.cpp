#include "app/viewport.hpp"

#include <algorithm>

namespace ss::app {

namespace {

// first так, чтобы value попал в [first, first + size), и окно не вылезло за лист
int scroll(int first, int size, int value, int limit) {
    if (value < first) {
        first = value;
    } else if (value >= first + size) {
        first = value - size + 1;
    }
    return std::clamp(first, 0, std::max(0, limit - size));
}

}  // namespace

int Viewport::visible_cols(const ss::core::ColumnWidths& widths) const {
    int count = 0;
    int used = 0;
    for (int col = first_col_; col < ss::core::max_cols; ++col) {
        used += widths.get(col);
        if (used > width_) {
            break;
        }
        ++count;
    }
    return std::max(1, count);
}

void Viewport::resize(int visible_rows, int width) {
    visible_rows_ = std::max(1, visible_rows);
    width_ = std::max(1, width);
}

int Viewport::leftmost_for(int last, const ss::core::ColumnWidths& widths) const {
    int first = last;
    int used = widths.get(last);
    while (first > 0 && used + widths.get(first - 1) <= width_) {
        --first;
        used += widths.get(first);
    }
    return first;
}

void Viewport::ensure_visible(ss::core::CellAddress cell, const ss::core::ColumnWidths& widths) {
    first_row_ = scroll(first_row_, visible_rows_, cell.row, ss::core::max_rows);
    if (cell.col < first_col_) {
        first_col_ = cell.col;
    } else {
        // если столбец не влезает справа, начало уезжает вправо
        first_col_ = std::max(first_col_, leftmost_for(cell.col, widths));
    }
    // окно не должно торчать за последний столбец листа
    first_col_ = std::min(first_col_, leftmost_for(ss::core::max_cols - 1, widths));
}

bool Viewport::contains(ss::core::CellAddress cell, const ss::core::ColumnWidths& widths) const {
    return cell.row >= first_row_ && cell.row < first_row_ + visible_rows_ &&
           cell.col >= first_col_ && cell.col < first_col_ + visible_cols(widths);
}

}  // namespace ss::app
