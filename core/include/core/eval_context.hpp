#pragma once

#include <vector>

#include "core/address.hpp"
#include "core/value.hpp"

namespace ss::core {

class Expr;

// Аргумент функции после вычисления. Из диапазона приходит много значений,
// из обычного аргумента одно. from_range нужен SUM и похожим функциям:
// строку из диапазона они пропускают, а строку-аргумент считают ошибкой.
struct ArgValues {
    std::vector<ValuePtr> values;
    bool from_range = false;
};

// То, что видят вычислитель и функции: значения ячеек и вычисление
// аргументов. Весь лист им не нужен.
class EvalContext {
public:
    virtual ~EvalContext() = default;
    EvalContext(const EvalContext&) = delete;
    EvalContext& operator=(const EvalContext&) = delete;
    EvalContext(EvalContext&&) = delete;
    EvalContext& operator=(EvalContext&&) = delete;

    // уже посчитанное значение ячейки, у пустой EmptyValue
    virtual ValuePtr value_at(CellAddress address) const = 0;

    // одно значение; диапазон тут даёт #VALUE!
    virtual ValuePtr evaluate(const Expr& expr) = 0;

    // диапазон разворачивается по строкам, остальное даёт одно значение
    virtual ArgValues evaluate_flat(const Expr& expr) = 0;

protected:
    EvalContext() = default;
};

}  // namespace ss::core
