#include "app/set_style_command.hpp"

#include <string>
#include <utility>

#include "core/errors.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

SetStyleCommand::SetStyleCommand(ss::core::Range range, Change change)
    : range_(range), change_(std::move(change)) {}

ss::core::DocumentEdit SetStyleCommand::plan(const ss::core::Spreadsheet& document) const {
    if (range_.size() > max_cells) {
        throw ss::core::LimitError("слишком большой диапазон для оформления, можно до " +
                                   std::to_string(max_cells) + " ячеек");
    }
    ss::core::DocumentEdit edit;
    range_.for_each([&](ss::core::CellAddress address) {
        const ss::core::CellContent before = document.content(address);
        ss::core::CellContent after = before;
        change_(after.style);
        if (after != before) {
            edit.cells.push_back({.address = address, .before = before, .after = after});
        }
    });
    return edit;
}

}  // namespace ss::app
