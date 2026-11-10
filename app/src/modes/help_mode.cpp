#include "app/modes/help_mode.hpp"

#include <memory>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/modes/normal_mode.hpp"
#include "app/session.hpp"

namespace ss::app {

std::string_view HelpMode::name() const {
    return "help";
}

void HelpMode::handle_key(Session& session, std::string_view key) const {
    const int page = session.viewport().visible_rows();
    if (key == "down") {
        session.set_help_scroll(session.help_scroll() + 1);
    } else if (key == "up") {
        session.set_help_scroll(session.help_scroll() - 1);
    } else if (key == "pagedown") {
        session.set_help_scroll(session.help_scroll() + page);
    } else if (key == "pageup") {
        session.set_help_scroll(session.help_scroll() - page);
    } else {
        session.set_mode(std::make_unique<NormalMode>());
    }
}

// строка вида "move-up [k, up]: курсор вверх"
std::vector<std::string> HelpMode::lines(const ActionRegistry& actions, const Keymap& keymap) {
    std::vector<std::string> result;
    for (const std::string& name : actions.names()) {
        std::string line = name;
        const std::vector<std::string> keys = keymap.keys_for(name);
        if (!keys.empty()) {
            line += " [";
            for (std::size_t i = 0; i < keys.size(); ++i) {
                line += (i == 0 ? "" : ", ") + keys[i];
            }
            line += "]";
        }
        line += ": ";
        line += actions.create(name)->description();
        result.push_back(std::move(line));
    }
    return result;
}

}  // namespace ss::app
