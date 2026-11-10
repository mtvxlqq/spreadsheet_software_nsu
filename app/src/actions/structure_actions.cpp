#include "actions/structure_actions.hpp"

#include <memory>
#include <utility>

#include "actions/number_argument.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"
#include "app/structure_command.hpp"

namespace ss::app {

using ss::core::Axis;
using ss::core::StructureEdit;

StructureAction::StructureAction(std::string name, std::string description,
                                 StructureEdit::Kind kind, Axis axis)
    : name_(std::move(name)), description_(std::move(description)), kind_(kind), axis_(axis) {}

std::string_view StructureAction::name() const {
    return name_;
}

std::string_view StructureAction::description() const {
    return description_;
}

void StructureAction::execute(Session& session, std::span<const std::string> args) const {
    if (args.size() > 1) {
        throw CommandError("нужно не больше одного числа: " + name_ + " [сколько]");
    }
    const ss::core::Range range = session.target_range();
    const bool rows = axis_ == Axis::Row;
    const int first = rows ? range.from().row : range.from().col;
    const int last = rows ? range.to().row : range.to().col;
    const int count = args.empty() ? last - first + 1 : number_argument(args.front());
    if (count < 1) {
        throw CommandError("количество должно быть не меньше 1");
    }
    session.apply_edit(std::make_unique<StructureCommand>(
        StructureEdit{.kind = kind_, .axis = axis_, .at = first, .count = count},
        session.functions()));
}

}  // namespace ss::app
