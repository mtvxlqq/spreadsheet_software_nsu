#include "actions/mode_actions.hpp"

#include <memory>

#include "actions/no_args.hpp"
#include "app/modes/command_mode.hpp"
#include "app/modes/help_mode.hpp"
#include "app/session.hpp"

namespace ss::app {

std::string_view CommandModeAction::name() const {
    return "command-mode";
}

std::string_view CommandModeAction::description() const {
    return "командная строка";
}

void CommandModeAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.edit_line().clear();
    session.set_mode(std::make_unique<CommandMode>());
}

std::string_view ShowHelpAction::name() const {
    return "show-help";
}

std::string_view ShowHelpAction::description() const {
    return "справка по клавишам и командам";
}

void ShowHelpAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.set_mode(std::make_unique<HelpMode>());
}

}  // namespace ss::app
