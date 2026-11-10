#include "app/clear_range_command.hpp"

#include "core/spreadsheet.hpp"

namespace ss::app {

ClearRangeCommand::ClearRangeCommand(ss::core::Range range) : range_(range) {}

ss::core::DocumentEdit ClearRangeCommand::plan(const ss::core::Spreadsheet& document) const {
    ss::core::DocumentEdit edit;
    // Обходим заполненные ячейки листа, а не весь диапазон: он бывает огромным.
    document.for_each_cell([&](ss::core::CellAddress address) {
        if (range_.contains(address)) {
            const ss::core::CellContent before = document.content(address);
            edit.cells.push_back(
                {.address = address, .before = before, .after = {.raw = "", .style = before.style}});
        }
    });
    return edit;
}

}  // namespace ss::app
