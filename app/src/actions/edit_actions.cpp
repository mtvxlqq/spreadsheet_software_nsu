#include "actions/edit_actions.hpp"

#include <memory>

#include "actions/no_args.hpp"
#include "app/modes/edit_mode.hpp"
#include "app/session.hpp"
#include "app/set_cell_command.hpp"

namespace ss::app {

std::string_view StartEditAction::name() const {
    return "start-edit";
}

std::string_view StartEditAction::description() const {
    return "править ячейку";
}

void StartEditAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.edit_line().set_text(session.document().raw(session.cursor()));
    session.set_mode(std::make_unique<EditMode>());
}

std::string_view ClearCellAction::name() const {
    return "clear-cell";
}

std::string_view ClearCellAction::description() const {
    return "очистить ячейку";
}

void ClearCellAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    // пустой ввод очищает ячейку, а через команду это потом можно будет отменить
    session.apply_edit(std::make_unique<SetCellCommand>(session.cursor(), ""));
}

}  // namespace ss::app
