#pragma once

#include "app/planned_command.hpp"
#include "core/address.hpp"

namespace ss::app {

// Очищает все ячейки диапазона. Оформление остаётся, как и при очистке
// одной ячейки.
class ClearRangeCommand final : public PlannedCommand {
public:
    explicit ClearRangeCommand(ss::core::Range range);

private:
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    ss::core::Range range_;
};

}  // namespace ss::app
