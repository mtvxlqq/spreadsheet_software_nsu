#pragma once

#include <string>
#include <string_view>

namespace ss::core {

// Регистр меняется только у латинских букв: имена функций, адреса и
// расширения файлов — ASCII, и разбор формул не должен зависеть от локали.
std::string toUpperAscii(std::string_view text);
std::string toLowerAscii(std::string_view text);

}  // namespace ss::core
