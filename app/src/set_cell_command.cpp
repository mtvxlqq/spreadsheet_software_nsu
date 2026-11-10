#include "app/set_cell_command.hpp"

#include <utility>

#include "core/spreadsheet.hpp"

namespace ss::app {

SetCellCommand::SetCellCommand(ss::core::CellAddress address, std::string raw)
    : address_(address), raw_(std::move(raw)) {}

ss::core::DocumentEdit SetCellCommand::plan(const ss::core::Spreadsheet& document) const {
    const ss::core::CellContent before = document.content(address_);
    ss::core::DocumentEdit edit;
    edit.cells.push_back(
        {.address = address_, .before = before, .after = {.raw = raw_, .style = before.style}});
    return edit;
}

}  // namespace ss::app
