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

void Viewport::resize(int visible_rows, int visible_cols) {
    visible_rows_ = std::max(1, visible_rows);
    visible_cols_ = std::max(1, visible_cols);
}

void Viewport::ensure_visible(ss::core::CellAddress cell) {
    first_row_ = scroll(first_row_, visible_rows_, cell.row, ss::core::max_rows);
    first_col_ = scroll(first_col_, visible_cols_, cell.col, ss::core::max_cols);
}

bool Viewport::contains(ss::core::CellAddress cell) const {
    return cell.row >= first_row_ && cell.row < first_row_ + visible_rows_ &&
           cell.col >= first_col_ && cell.col < first_col_ + visible_cols_;
}

}  // namespace ss::app
