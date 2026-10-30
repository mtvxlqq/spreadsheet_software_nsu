#pragma once

#include <ostream>

#include "core/address.hpp"

namespace ss::core {

// Чтобы GoogleTest печатал адреса в сообщениях о провале как "B12",
// а не как набор байтов.
inline void PrintTo(const CellAddress& address, std::ostream* os) {
    *os << address.toString();
}

inline void PrintTo(const Range& range, std::ostream* os) {
    *os << range.toString();
}

}  // namespace ss::core
