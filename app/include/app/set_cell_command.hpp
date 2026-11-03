#pragma once

#include <string>

#include "app/edit_command.hpp"
#include "core/address.hpp"

namespace ss::app {

// Записать в ячейку то, что ввёл пользователь. Пустой raw очищает ячейку.
class SetCellCommand final : public EditCommand {
public:
    SetCellCommand(ss::core::CellAddress address, std::string raw);

    // запоминает старый raw, чтобы revert мог его вернуть
    void apply(ss::core::Spreadsheet& document) override;
    void revert(ss::core::Spreadsheet& document) override;

private:
    ss::core::CellAddress address_;
    std::string raw_;
    std::string old_raw_;
};

}  // namespace ss::app
