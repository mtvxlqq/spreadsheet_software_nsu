#include "actions/width_actions.hpp"

#include <memory>
#include <utility>

#include "actions/no_args.hpp"
#include "actions/number_argument.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"
#include "app/set_column_width_command.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

namespace {

// ширину получают все столбцы, с которыми работает команда
void set_width(Session& session, int width) {
    const ss::core::Range range = session.target_range();
    session.apply_edit(
        std::make_unique<SetColumnWidthCommand>(range.from().col, range.to().col, width));
}

}  // namespace

ColumnWidthStepAction::ColumnWidthStepAction(std::string name, std::string description, int delta)
    : name_(std::move(name)), description_(std::move(description)), delta_(delta) {}

std::string_view ColumnWidthStepAction::name() const {
    return name_;
}

std::string_view ColumnWidthStepAction::description() const {
    return description_;
}

void ColumnWidthStepAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name_, args);
    const int width = session.document().column_widths().get(session.cursor().col);
    set_width(session, width + delta_);
}

std::string_view WidthAction::name() const {
    return "width";
}

std::string_view WidthAction::description() const {
    return "ширина столбца (:width 15)";
}

void WidthAction::execute(Session& session, std::span<const std::string> args) const {
    if (args.size() != 1) {
        throw CommandError("нужно :width число, например :width 15");
    }
    set_width(session, number_argument(args.front()));
}

}  // namespace ss::app
