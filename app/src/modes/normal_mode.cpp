#include "app/modes/normal_mode.hpp"

#include <memory>
#include <optional>
#include <string>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/modes/edit_mode.hpp"
#include "app/session.hpp"

namespace ss::app {

std::string_view NormalMode::name() const {
    return "normal";
}

void NormalMode::handle_key(Session& session, std::string_view key) const {
    if (const std::optional<std::string> action = session.keymap().action_for(key)) {
        session.actions().create(*action)->execute(session, {});
        return;
    }
    // как в Excel: просто начинаем печатать, и старое содержимое заменяется
    if (is_text_key(key)) {
        session.edit_line().set_text(std::string(key));
        session.set_mode(std::make_unique<EditMode>());
    }
}

}  // namespace ss::app
