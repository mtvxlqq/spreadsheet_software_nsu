#include "app/set_cell_command.hpp"

#include <utility>

#include "core/spreadsheet.hpp"

namespace ss::app {

SetCellCommand::SetCellCommand(ss::core::CellAddress address, std::string raw)
    : address_(address), raw_(std::move(raw)) {}

void SetCellCommand::apply(ss::core::Spreadsheet& document) {
    std::string old_raw = document.raw(address_);
    // set_input даёт строгую гарантию, а old_raw_ меняем только после успеха
    document.set_input(address_, raw_);
    old_raw_ = std::move(old_raw);
}

void SetCellCommand::revert(ss::core::Spreadsheet& document) {
    // старый raw уже однажды разобрался, так что тут не бросит
    document.set_input(address_, old_raw_);
}

}  // namespace ss::app
