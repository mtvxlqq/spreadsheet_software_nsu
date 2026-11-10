#include "core/column_widths.hpp"

#include <string>

#include "core/address.hpp"
#include "core/errors.hpp"

namespace ss::core {

int ColumnWidths::get(int col) const {
    const auto it = widths_.find(col);
    return it == widths_.end() ? default_column_width : it->second;
}

void ColumnWidths::set(int col, int width) {
    if (col < 0 || col >= max_cols) {
        throw LimitError("столбец за пределами листа");
    }
    if (width < min_column_width || width > max_column_width) {
        throw LimitError("ширина столбца может быть от " + std::to_string(min_column_width) +
                         " до " + std::to_string(max_column_width));
    }
    if (width == default_column_width) {
        widths_.erase(col);
        return;
    }
    widths_.insert_or_assign(col, width);
}

}  // namespace ss::core
