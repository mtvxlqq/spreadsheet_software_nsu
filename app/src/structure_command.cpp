#include "app/structure_command.hpp"

#include "core/structure_edit.hpp"

namespace ss::app {

StructureCommand::StructureCommand(ss::core::StructureEdit edit,
                                   const ss::core::FunctionProvider& functions)
    : edit_(edit), functions_(functions) {}

ss::core::DocumentEdit StructureCommand::plan(const ss::core::Spreadsheet& document) const {
    return ss::core::plan_structure_edit(document, functions_, edit_);
}

}  // namespace ss::app
