#pragma once

#include "app/clipboard.hpp"
#include "app/planned_command.hpp"
#include "core/address.hpp"

namespace ss::core {
class FunctionProvider;
}

namespace ss::app {

// Вставляет скопированный диапазон так, чтобы его левый верхний угол попал
// в target. Относительные ссылки в формулах сдвигаются на то же расстояние,
// ссылки с $ остаются. Куда попали пустые ячейки диапазона, там очищается.
class PasteCommand final : public PlannedCommand {
public:
    // functions должен жить дольше команды
    PasteCommand(Clipboard clipboard, ss::core::CellAddress target,
                 const ss::core::FunctionProvider& functions);

private:
    // диапазон не помещается в лист: LimitError
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    Clipboard clipboard_;
    ss::core::CellAddress target_;
    const ss::core::FunctionProvider& functions_;
};

}  // namespace ss::app
