#include "app/planned_command.hpp"

#include <utility>

#include "core/spreadsheet.hpp"

namespace ss::app {

void PlannedCommand::apply(ss::core::Spreadsheet& document) {
    ss::core::DocumentEdit edit = plan(document);
    document.apply(edit);
    // запоминаем только после успеха, иначе revert вернул бы то, чего не было
    edit_ = std::move(edit);
}

void PlannedCommand::revert(ss::core::Spreadsheet& document) {
    document.apply(edit_.inverse());
}

}  // namespace ss::app
