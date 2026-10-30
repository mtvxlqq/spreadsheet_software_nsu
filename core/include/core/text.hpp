#pragma once

#include <string>
#include <string_view>

namespace ss::core {

// Меняем регистр только у латиницы: имена функций и адреса всегда ASCII,
// а std::toupper зависит от локали.
std::string to_upper_ascii(std::string_view text);
std::string to_lower_ascii(std::string_view text);

}  // namespace ss::core
