#pragma once

#include <span>
#include <string_view>

#include "core/function.hpp"

namespace ss::core {

// IF(условие, если_да [, если_нет]). Считается только нужная ветка, поэтому IF(1, 1, 1/0) даёт 1.
class IfFunction final : public Function {
public:
    std::string_view name() const override;
    ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const override;
};

}  // namespace ss::core
