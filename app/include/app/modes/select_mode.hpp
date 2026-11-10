#pragma once

#include "app/mode.hpp"

namespace ss::app {

// Выделение диапазона: от ячейки, где его начали, до курсора. Клавиши
// работают как в обычном режиме, только печатные символы правку не начинают.
// Перемещения растягивают выделение, любое другое действие работает
// с выделенным диапазоном и возвращает в NormalMode. esc просто снимает его.
class SelectMode final : public Mode {
public:
    SelectMode() = default;

    std::string_view name() const override;
    void handle_key(Session& session, std::string_view key) const override;

    bool keeps_selection() const override {
        return true;
    }
};

}  // namespace ss::app
