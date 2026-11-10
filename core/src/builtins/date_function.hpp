#pragma once

#include <span>
#include <string_view>

#include "core/function.hpp"

namespace ss::core {

// DATE(год, месяц, день) даёт дату. Если такой даты нет, например
// DATE(2024, 2, 31), получится #VALUE!.
class DateFunction final : public Function {
public:
    std::string_view name() const override;
    ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const override;
};

}  // namespace ss::core
