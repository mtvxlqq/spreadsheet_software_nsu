#include "builtins/if_function.hpp"

#include <optional>

#include "core/eval_context.hpp"

namespace ss::core {

std::string_view IfFunction::name() const {
    return "IF";
}

ValuePtr IfFunction::call(std::span<const Expr* const> args, EvalContext& context) const {
    if (args.size() < 2 || args.size() > 3) {
        return make_error(ErrorCode::Value);
    }
    const ValuePtr condition = context.evaluate(*args[0]);
    if (condition->is_error()) {
        return condition;
    }
    // пустая ячейка считается нулём, то есть ложью
    const std::optional<double> number = condition->as_number();
    if (!number) {
        return make_error(ErrorCode::Value);
    }
    if (*number != 0) {
        return context.evaluate(*args[1]);
    }
    if (args.size() == 3) {
        return context.evaluate(*args[2]);
    }
    return make_number(0);
}

}  // namespace ss::core
