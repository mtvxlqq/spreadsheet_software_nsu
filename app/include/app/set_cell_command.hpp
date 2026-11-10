#pragma once

#include <string>

#include "app/planned_command.hpp"
#include "core/address.hpp"

namespace ss::app {

// Записать в ячейку то, что ввёл пользователь. Пустой raw очищает ячейку,
// оформление ячейки остаётся.
class SetCellCommand final : public PlannedCommand {
public:
    SetCellCommand(ss::core::CellAddress address, std::string raw);

private:
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    ss::core::CellAddress address_;
    std::string raw_;
};

}  // namespace ss::app
