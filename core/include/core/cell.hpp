#pragma once

#include <string>

#include "core/expr.hpp"
#include "core/value.hpp"

namespace ss::core {

// Заполненная ячейка. Оформления тут нет, оно лежит отдельно в StyleTable.
struct Cell {
    // как ввёл пользователь, например "=a1 + 1" или "'12"
    std::string raw;
    // nullptr, если в ячейке просто число или текст
    ExprPtr formula;
    // у формулы это последний результат вычисления
    ValuePtr value;
};

}  // namespace ss::core
