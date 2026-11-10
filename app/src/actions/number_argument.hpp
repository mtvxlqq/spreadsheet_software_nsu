#pragma once

#include <charconv>
#include <string>
#include <string_view>
#include <system_error>

#include "app/errors.hpp"

namespace ss::app {

// целое число из аргумента команды; на "abc" и "1.5" бросает CommandError
inline int number_argument(std::string_view text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw CommandError("нужно целое число, а не " + std::string(text));
    }
    return value;
}

}  // namespace ss::app
