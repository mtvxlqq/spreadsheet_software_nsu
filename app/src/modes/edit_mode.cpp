#include "app/modes/edit_mode.hpp"

#include <memory>

#include "app/modes/normal_mode.hpp"
#include "app/session.hpp"
#include "app/set_cell_command.hpp"

namespace ss::app {

namespace {

// Если формула битая, apply_edit бросит ParseError ещё до того, как мы
// очистим строку и сменим режим. Так набранный текст не пропадёт.
void commit(Session& session, int d_row, int d_col) {
    session.apply_edit(
        std::make_unique<SetCellCommand>(session.cursor(), session.edit_line().text()));
    session.edit_line().clear();
    session.move_cursor(d_row, d_col);
    session.set_mode(std::make_unique<NormalMode>());
}

}  // namespace

std::string_view EditMode::name() const {
    return "edit";
}

void EditMode::handle_key(Session& session, std::string_view key) const {
    if (key == "enter") {
        commit(session, 1, 0);
    } else if (key == "tab") {
        commit(session, 0, 1);
    } else if (key == "esc") {
        session.edit_line().clear();
        session.set_mode(std::make_unique<NormalMode>());
    } else {
        // остальные клавиши, которые строка не поняла, просто игнорируем
        session.edit_line().handle_key(key);
    }
}

}  // namespace ss::app
