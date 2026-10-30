#pragma once

#include <ostream>

#include "core/address.hpp"
#include "core/token.hpp"

namespace ss::core {

// Чтобы gtest при падении теста печатал "B12", а не байты.
// Имя PrintTo требует сам gtest.
// NOLINTBEGIN(readability-identifier-naming)

inline void PrintTo(const CellAddress& address, std::ostream* os) {
    *os << address.to_string();
}

inline void PrintTo(const CellRef& ref, std::ostream* os) {
    *os << ref.to_string();
}

inline void PrintTo(const Range& range, std::ostream* os) {
    *os << range.to_string();
}

inline void PrintTo(const Token& token, std::ostream* os) {
    *os << "{type=" << static_cast<int>(token.type) << ", text=\"" << token.text
        << "\", pos=" << token.pos << "}";
}

// NOLINTEND(readability-identifier-naming)

}  // namespace ss::core
