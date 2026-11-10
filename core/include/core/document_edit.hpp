#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/address.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"

namespace ss::core {

// всё, что хранится в одной ячейке
struct CellContent {
    // как ввёл пользователь, пустая строка значит пустую ячейку
    std::string raw;
    CellStyle style;

    bool operator==(const CellContent&) const = default;
};

struct CellChange {
    CellAddress address;
    CellContent before;
    CellContent after;
};

// Правка документа, посчитанная заранее. Её применяет Spreadsheet::apply,
// а отменить её значит применить inverse(): before и after меняются местами.
// Так отменяется даже удаление строки, которое обратной операцией не вернёшь.
struct DocumentEdit {
    std::vector<CellChange> cells;
    // заполнены, только если правка меняет ширины столбцов
    std::optional<ColumnWidths> widths_before;
    std::optional<ColumnWidths> widths_after;

    DocumentEdit inverse() const;
};

}  // namespace ss::core
