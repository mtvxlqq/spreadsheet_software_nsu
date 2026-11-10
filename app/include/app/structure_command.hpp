#pragma once

#include "app/planned_command.hpp"
#include "core/ref_mapping.hpp"

namespace ss::core {
class FunctionProvider;
}

namespace ss::app {

// Вставка или удаление строк и столбцов. Саму правку считает ядро
// (plan_structure_edit), а отмена достаётся от PlannedCommand: в правке
// записано старое содержимое всех тронутых ячеек, включая удалённые.
class StructureCommand final : public PlannedCommand {
public:
    // functions должен жить дольше команды
    StructureCommand(ss::core::StructureEdit edit, const ss::core::FunctionProvider& functions);

private:
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override;

    ss::core::StructureEdit edit_;
    const ss::core::FunctionProvider& functions_;
};

}  // namespace ss::app
