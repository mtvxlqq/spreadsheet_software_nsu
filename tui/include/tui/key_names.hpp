#pragma once

#include <optional>
#include <string>

#include <ftxui/component/event.hpp>

namespace ss::tui {

// Имя клавиши для Session::handle_key: "up", "enter", "ctrl+s", "ж".
// nullopt для всего, что не клавиша или что мы не знаем: терминал присылает
// ответы на свои запросы теми же событиями, и в сеанс они попасть не должны.
std::optional<std::string> key_name(const ftxui::Event& event);

}  // namespace ss::tui
