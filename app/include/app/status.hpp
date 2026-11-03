#pragma once

#include <string>

namespace ss::app {

// сообщение в строке состояния
struct Status {
    enum class Kind { Info, Error };

    std::string text;
    Kind kind = Kind::Info;
};

}  // namespace ss::app
