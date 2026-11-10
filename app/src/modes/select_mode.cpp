#include "app/modes/select_mode.hpp"

#include <memory>
#include <optional>
#include <string>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/modes/normal_mode.hpp"
#include "app/session.hpp"

namespace ss::app {

std::string_view SelectMode::name() const {
    return "select";
}

void SelectMode::handle_key(Session& session, std::string_view key) const {
    if (key == "esc") {
        session.set_mode(std::make_unique<NormalMode>());
        return;
    }
    const std::optional<std::string> name = session.keymap().action_for(key);
    if (!name) {
        return;
    }
    const std::unique_ptr<Action> action = session.actions().create(*name);
    if (!action->keeps_selection()) {
        // Normal ставим до execute: действие может само включить другой режим.
        // Выделение при этом ещё на месте, сеанс снимет его после клавиши.
        // Если действие бросит, останемся здесь с тем же выделением.
        session.set_mode(std::make_unique<NormalMode>());
    }
    action->execute(session, {});
}

}  // namespace ss::app
