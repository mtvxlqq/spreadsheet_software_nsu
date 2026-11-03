#pragma once

#include "core/errors.hpp"

namespace ss::app {

// Ошибки app наследуются от ss::core::Error, поэтому Session ловит
// их тем же catch, что и ошибки ядра.

// неправильная строка в конфиге клавиш, например кривое имя клавиши
class ConfigError : public ss::core::Error {
public:
    using ss::core::Error::Error;
};

// неизвестная команда вроде ":xyz" или не хватает аргумента
class CommandError : public ss::core::Error {
public:
    using ss::core::Error::Error;
};

}  // namespace ss::app
