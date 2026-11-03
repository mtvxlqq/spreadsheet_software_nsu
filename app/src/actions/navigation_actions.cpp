#include "actions/navigation_actions.hpp"

#include <utility>

#include "actions/no_args.hpp"
#include "app/session.hpp"

namespace ss::app {

MoveAction::MoveAction(std::string name, std::string description, int d_row, int d_col)
    : name_(std::move(name)), description_(std::move(description)), d_row_(d_row), d_col_(d_col) {}

std::string_view MoveAction::name() const {
    return name_;
}

std::string_view MoveAction::description() const {
    return description_;
}

void MoveAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name_, args);
    session.move_cursor(d_row_, d_col_);
}

PageAction::PageAction(std::string name, std::string description, int direction)
    : name_(std::move(name)), description_(std::move(description)), direction_(direction) {}

std::string_view PageAction::name() const {
    return name_;
}

std::string_view PageAction::description() const {
    return description_;
}

void PageAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name_, args);
    session.move_cursor(direction_ * session.viewport().visible_rows(), 0);
}

std::string_view GoHomeAction::name() const {
    return "go-home";
}

std::string_view GoHomeAction::description() const {
    return "в начало строки (столбец A)";
}

void GoHomeAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.set_cursor({.row = session.cursor().row, .col = 0});
}

std::string_view GoTopAction::name() const {
    return "go-top";
}

std::string_view GoTopAction::description() const {
    return "в ячейку A1";
}

void GoTopAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.set_cursor({.row = 0, .col = 0});
}

}  // namespace ss::app
