#include "actions/quit_actions.hpp"

#include "actions/no_args.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"

namespace ss::app {

std::string_view QuitAction::name() const {
    return "quit";
}

std::string_view QuitAction::description() const {
    return "выйти (:q)";
}

void QuitAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    if (session.dirty()) {
        throw CommandError("есть несохранённые изменения, :q! для выхода");
    }
    session.request_quit();
}

std::string_view ForceQuitAction::name() const {
    return "force-quit";
}

std::string_view ForceQuitAction::description() const {
    return "выйти без сохранения (:q!)";
}

void ForceQuitAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.request_quit();
}

}  // namespace ss::app
