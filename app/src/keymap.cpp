#include "app/keymap.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <string>

#include "app/errors.hpp"
#include "core/text.hpp"

namespace ss::app {

namespace {

using ss::core::to_lower_ascii;
using ss::core::trim_spaces;

constexpr std::string_view ctrl_prefix = "ctrl+";

constexpr std::array<std::string_view, 13> named_keys = {
    "up",     "down", "left", "right", "enter",  "esc",      "backspace",
    "delete", "tab",  "home", "end",   "pageup", "pagedown",
};

bool is_named_key(std::string_view key) {
    if (std::ranges::find(named_keys, key) != named_keys.end()) {
        return true;
    }
    for (int n = 1; n <= 12; ++n) {
        if (key == "f" + std::to_string(n)) {
            return true;
        }
    }
    return false;
}

// сколько байт занимает символ UTF-8 с таким первым байтом, 0 если байт кривой
std::size_t utf8_length(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead >> 5) == 0x6) {
        return 2;
    }
    if ((lead >> 4) == 0xE) {
        return 3;
    }
    if ((lead >> 3) == 0x1E) {
        return 4;
    }
    return 0;
}

// ровно один печатный символ UTF-8, например "h", "?" или "ж"
bool is_single_char(std::string_view key) {
    if (key.empty()) {
        return false;
    }
    const auto lead = static_cast<unsigned char>(key.front());
    const std::size_t length = utf8_length(lead);
    if (length == 0 || length != key.size()) {
        return false;
    }
    if (length == 1) {
        return lead >= 0x20 && lead != 0x7F;  // без управляющих символов
    }
    return std::ranges::all_of(key.substr(1), [](char c) {
        return (static_cast<unsigned char>(c) >> 6) == 0x2;
    });
}

// одиночный символ оставляем как есть ("H" и "h" это разные клавиши),
// у имён вроде "PageUp" регистр не важен
std::optional<std::string> normalize_base(std::string_view key) {
    if (is_single_char(key)) {
        return std::string(key);
    }
    std::string lower = to_lower_ascii(key);
    if (is_named_key(lower)) {
        return lower;
    }
    return std::nullopt;
}

}  // namespace

Keymap Keymap::defaults() {
    Keymap keymap;
    keymap.bind("up", "move-up");
    keymap.bind("k", "move-up");
    keymap.bind("down", "move-down");
    keymap.bind("j", "move-down");
    keymap.bind("left", "move-left");
    keymap.bind("h", "move-left");
    keymap.bind("right", "move-right");
    keymap.bind("l", "move-right");
    keymap.bind("pageup", "page-up");
    keymap.bind("pagedown", "page-down");
    keymap.bind("home", "go-home");
    keymap.bind("ctrl+home", "go-top");
    keymap.bind("enter", "start-edit");
    keymap.bind("f2", "start-edit");
    keymap.bind("delete", "clear-cell");
    keymap.bind(":", "command-mode");
    keymap.bind("?", "show-help");
    keymap.bind("f1", "show-help");
    keymap.bind("ctrl+s", "save");
    keymap.bind("ctrl+q", "quit");
    return keymap;
}

KeymapLoadResult Keymap::load(const std::filesystem::path& path,
                              const std::vector<std::string>& known_actions) {
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        return {.keymap = defaults(), .warnings = {}};
    }
    std::ifstream in(path);
    if (!std::filesystem::is_regular_file(path, error) || !in) {
        return {.keymap = defaults(), .warnings = {"не удалось открыть " + path.string()}};
    }
    KeymapLoadResult result = load(in, known_actions);
    if (in.bad()) {
        result.warnings.push_back("не удалось дочитать " + path.string());
    }
    return result;
}

KeymapLoadResult Keymap::load(std::istream& in, const std::vector<std::string>& known_actions) {
    KeymapLoadResult result{.keymap = defaults(), .warnings = {}};
    std::string line;
    std::size_t number = 0;
    const auto warn = [&](const std::string& message) {
        result.warnings.push_back("строка " + std::to_string(number) + ": " + message);
    };
    while (std::getline(in, line)) {
        ++number;
        const std::string_view text = trim_spaces(line);
        if (text.empty() || text.front() == '#') {
            continue;
        }
        // Ищем последний '=', чтобы можно было назначить саму клавишу "=":
        // строка "= = save"
        const std::size_t equals = text.rfind('=');
        if (equals == std::string_view::npos) {
            warn("нет '=', нужно писать клавиша = действие");
            continue;
        }
        const std::string_view key = trim_spaces(text.substr(0, equals));
        const std::string action(trim_spaces(text.substr(equals + 1)));
        if (key.empty()) {
            warn("не указана клавиша");
        } else if (action.empty()) {
            warn("не указано действие");
        } else if (std::ranges::find(known_actions, action) == known_actions.end()) {
            warn("неизвестное действие " + action);
        } else {
            try {
                result.keymap.bind(key, action);
            } catch (const ConfigError& error) {
                warn(error.what());
            }
        }
    }
    return result;
}

std::optional<std::string> Keymap::normalize_key(std::string_view key) {
    if (key.size() > ctrl_prefix.size() && to_lower_ascii(key).starts_with(ctrl_prefix)) {
        const std::optional<std::string> base = normalize_base(key.substr(ctrl_prefix.size()));
        if (!base) {
            return std::nullopt;
        }
        // ctrl+S и ctrl+s это одно и то же
        return std::string(ctrl_prefix) + to_lower_ascii(*base);
    }
    return normalize_base(key);
}

void Keymap::bind(std::string_view key, std::string action) {
    std::optional<std::string> normalized = normalize_key(key);
    if (!normalized) {
        throw ConfigError("непонятная клавиша " + std::string(key));
    }
    actions_.insert_or_assign(std::move(*normalized), std::move(action));
}

std::optional<std::string> Keymap::action_for(std::string_view key) const {
    const std::optional<std::string> normalized = normalize_key(key);
    if (!normalized) {
        return std::nullopt;
    }
    const auto it = actions_.find(*normalized);
    if (it == actions_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> Keymap::keys_for(std::string_view action) const {
    std::vector<std::string> keys;
    for (const auto& [key, bound_action] : actions_) {
        if (bound_action == action) {
            keys.push_back(key);
        }
    }
    return keys;
}

}  // namespace ss::app
