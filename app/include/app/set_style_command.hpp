#pragma once

#include <cstddef>
#include <functional>

#include "app/planned_command.hpp"
#include "core/address.hpp"
#include "core/cell_style.hpp"

namespace ss::app {

// Меняет оформление всех ячеек диапазона, содержимое не трогает. Что именно
// поменять в стиле, говорит функция change: формату нужно одно поле,
// выравниванию другое, а обход диапазона и отмена у них общие.
class SetStyleCommand final : public PlannedCommand {
public:
    using Change = std::function<void(ss::core::CellStyle&)>;

    // стиль бывает и у пустых ячеек, поэтому обходим весь диапазон,
    // а слишком большой не берём
    static constexpr std::size_t max_cells = 100'000;

    SetStyleCommand(ss::core::Range range, Change change);

private:
    // на диапазон больше max_cells бросает LimitError
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    ss::core::Range range_;
    Change change_;
};

}  // namespace ss::app
