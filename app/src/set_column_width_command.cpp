#include "app/set_column_width_command.hpp"

#include "core/column_widths.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

SetColumnWidthCommand::SetColumnWidthCommand(int first_col, int last_col, int width)
    : first_col_(first_col), last_col_(last_col), width_(width) {}

ss::core::DocumentEdit SetColumnWidthCommand::plan(const ss::core::Spreadsheet& document) const {
    ss::core::ColumnWidths widths = document.column_widths();
    for (int col = first_col_; col <= last_col_; ++col) {
        widths.set(col, width_);
    }
    ss::core::DocumentEdit edit;
    edit.widths_before = document.column_widths();
    edit.widths_after = widths;
    return edit;
}

}  // namespace ss::app
