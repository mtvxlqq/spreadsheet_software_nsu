#pragma once

#include <span>
#include <string>
#include <string_view>

#include "app/errors.hpp"

namespace ss::app {

// для действий без аргументов, чтобы ":go-top 5" не молча делал go-top
inline void expect_no_args(std::string_view name, std::span<const std::string> args) {
    if (!args.empty()) {
        throw CommandError("команда " + std::string(name) + " не принимает аргументов");
    }
}

}  // namespace ss::app
