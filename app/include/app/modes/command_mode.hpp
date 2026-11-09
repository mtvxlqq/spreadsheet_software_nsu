#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "app/mode.hpp"

namespace ss::app {

// Командная строка после ':'. По enter строка делится на слова: первое
// это имя действия (или алиас), остальные его аргументы.
class CommandMode final : public Mode {
public:
    CommandMode() = default;

    std::string_view name() const override;
    void handle_key(Session& session, std::string_view key) const override;

    bool uses_command_line() const override {
        return true;
    }

    // слова через пробелы, пустых не бывает
    static std::vector<std::string> split_words(std::string_view line);
};

}  // namespace ss::app
