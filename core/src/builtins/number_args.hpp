#pragma once

#include <span>
#include <vector>

#include "core/value.hpp"

namespace ss::core {

class EvalContext;
class Expr;

// Числа из аргументов SUM, MIN, MAX и AVERAGE. Если error не пустой,
// функция должна вернуть его вместо результата.
struct NumberArgs {
    std::vector<double> numbers;
    ValuePtr error;
};

// Правила общие для всех четырёх функций:
// - пустые ячейки пропускаются;
// - строка из диапазона пропускается, а строка-аргумент даёт #VALUE!;
// - первая встреченная ошибка возвращается как есть;
// - вызов без аргументов даёт #VALUE!
NumberArgs collect_numbers(std::span<const Expr* const> args, EvalContext& context);

}  // namespace ss::core
