#pragma once

#include "app/planned_command.hpp"

namespace ss::app {

// одна и та же ширина для столбцов с first_col по last_col
class SetColumnWidthCommand final : public PlannedCommand {
public:
    SetColumnWidthCommand(int first_col, int last_col, int width);

private:
    // недопустимая ширина даёт LimitError
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    int first_col_;
    int last_col_;
    int width_;
};

}  // namespace ss::app
