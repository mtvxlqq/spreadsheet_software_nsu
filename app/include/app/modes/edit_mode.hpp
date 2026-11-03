#pragma once

#include "app/mode.hpp"

namespace ss::app {

// Правка ячейки под курсором, текст лежит в session.edit_line().
// enter записывает и переходит вниз, esc отменяет. Если формула битая,
// остаёмся тут же, ошибка будет в статусе, а набранное не пропадёт.
class EditMode final : public Mode {
public:
    EditMode() = default;

    std::string_view name() const override;
    void handle_key(Session& session, std::string_view key) const override;

    bool edits_cell() const override {
        return true;
    }
};

}  // namespace ss::app
