#pragma once

#include <span>
#include <string_view>

#include "core/function.hpp"

namespace ss::core {

// AVERAGE(A1:A3): среднее, если чисел нет, то #DIV/0!
class AverageFunction final : public Function {
public:
    std::string_view name() const override;
    ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const override;
};

}  // namespace ss::core
