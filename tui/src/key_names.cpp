#include "tui/key_names.hpp"

#include <array>
#include <string_view>

#include "app/keymap.hpp"

namespace ss::tui {

namespace {

using ftxui::Event;

struct NamedKey {
    Event event;
    std::string_view name;
};

// в функции, а не глобально: Event::Return и другие сами глобальные объекты,
// и порядок их создания относительно наших был бы не определён
const std::array<NamedKey, 31>& named_keys() {
    static const std::array<NamedKey, 31> keys{{
        {.event = Event::ArrowUp, .name = "up"},
        {.event = Event::ArrowDown, .name = "down"},
        {.event = Event::ArrowLeft, .name = "left"},
        {.event = Event::ArrowRight, .name = "right"},
        {.event = Event::Return, .name = "enter"},
        {.event = Event::Escape, .name = "esc"},
        {.event = Event::Backspace, .name = "backspace"},
        {.event = Event::Delete, .name = "delete"},
        {.event = Event::Tab, .name = "tab"},
        {.event = Event::Home, .name = "home"},
        {.event = Event::End, .name = "end"},
        {.event = Event::PageUp, .name = "pageup"},
        {.event = Event::PageDown, .name = "pagedown"},
        {.event = Event::F1, .name = "f1"},
        {.event = Event::F2, .name = "f2"},
        {.event = Event::F3, .name = "f3"},
        {.event = Event::F4, .name = "f4"},
        {.event = Event::F5, .name = "f5"},
        {.event = Event::F6, .name = "f6"},
        {.event = Event::F7, .name = "f7"},
        {.event = Event::F8, .name = "f8"},
        {.event = Event::F9, .name = "f9"},
        {.event = Event::F10, .name = "f10"},
        {.event = Event::F11, .name = "f11"},
        {.event = Event::F12, .name = "f12"},
        {.event = Event::ArrowUpCtrl, .name = "ctrl+up"},
        {.event = Event::ArrowDownCtrl, .name = "ctrl+down"},
        {.event = Event::ArrowLeftCtrl, .name = "ctrl+left"},
        {.event = Event::ArrowRightCtrl, .name = "ctrl+right"},
        // для этих двух в FTXUI нет готовых констант
        {.event = Event::Special("\x1B[1;5H"), .name = "ctrl+home"},
        {.event = Event::Special("\x1B[1;5F"), .name = "ctrl+end"},
    }};
    return keys;
}

// Ctrl+A это байт 1, Ctrl+Z байт 26. Tab и Enter попадают в тот же
// диапазон, поэтому сначала проверяем именованные клавиши.
std::optional<std::string> ctrl_letter(const std::string& input) {
    if (input.size() != 1 || input[0] < 1 || input[0] > 26) {
        return std::nullopt;
    }
    return "ctrl+" + std::string(1, static_cast<char>('a' + input[0] - 1));
}

}  // namespace

std::optional<std::string> key_name(const Event& event) {
    if (event.is_character()) {
        std::string text = event.character();
        if (ss::app::is_text_key(text)) {
            return text;
        }
        return std::nullopt;
    }
    for (const NamedKey& key : named_keys()) {
        if (event == key.event) {
            return std::string(key.name);
        }
    }
    return ctrl_letter(event.input());
}

}  // namespace ss::tui
