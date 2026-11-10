#include "actions/edit_actions.hpp"

#include <memory>

#include "actions/no_args.hpp"
#include "app/clear_range_command.hpp"
#include "app/modes/edit_mode.hpp"
#include "app/session.hpp"

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
    return "очистить ячейку или выделенное";
}

void ClearCellAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.apply_edit(std::make_unique<ClearRangeCommand>(session.target_range()));
}

std::string_view UndoAction::name() const {
    return "undo";
}

std::string_view UndoAction::description() const {
    return "отменить правку";
}

void UndoAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    if (!session.undo()) {
        session.set_status("нечего отменять");
    }
}

std::string_view RedoAction::name() const {
    return "redo";
}

std::string_view RedoAction::description() const {
    return "повторить отменённое";
}

void RedoAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    if (!session.redo()) {
        session.set_status("нечего повторять");
    }
}

}  // namespace ss::app
