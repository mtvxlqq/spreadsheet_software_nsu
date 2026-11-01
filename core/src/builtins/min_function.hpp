#pragma once

#include <span>
#include <string_view>

#include "core/function.hpp"

namespace ss::core {

// MIN(A1:A3, 5): наименьшее число, если чисел нет, то 0
class MinFunction final : public Function {
public:
    std::string_view name() const override;
    ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const override;
};

}  // namespace ss::core
