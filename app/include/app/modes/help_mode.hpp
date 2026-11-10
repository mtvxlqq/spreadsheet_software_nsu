#pragma once

#include <string>
#include <vector>

#include "app/mode.hpp"

namespace ss::app {

class ActionRegistry;
class Keymap;

// Показывается справка. Стрелки и pageup/pagedown её листают, любая
// другая клавиша возвращает в NormalMode.
class HelpMode final : public Mode {
public:
    HelpMode() = default;

    std::string_view name() const override;
    void handle_key(Session& session, std::string_view key) const override;

    bool shows_help() const override {
        return true;
    }

    // по строке на действие: клавиши, имя и описание. Справка собирается
    // из реестра, так что новое действие попадёт в неё само.
    static std::vector<std::string> lines(const ActionRegistry& actions, const Keymap& keymap);
};

}  // namespace ss::app
