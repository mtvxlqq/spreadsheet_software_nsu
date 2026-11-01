#include "builtins/number_args.hpp"

#include <optional>

#include "core/eval_context.hpp"

namespace ss::core {

NumberArgs collect_numbers(std::span<const Expr* const> args, EvalContext& context) {
    NumberArgs result;
    if (args.empty()) {
        result.error = make_error(ErrorCode::Value);
        return result;
    }
    for (const Expr* arg : args) {
        const ArgValues arg_values = context.evaluate_flat(*arg);
        for (const ValuePtr& value : arg_values.values) {
            if (value->is_error()) {
                result.error = value;
                return result;
            }
            if (value->is_empty()) {
                continue;
            }
            const std::optional<double> number = value->as_number();
            if (number) {
                result.numbers.push_back(*number);
            } else if (!arg_values.from_range) {
                result.error = make_error(ErrorCode::Value);
                return result;
            }
        }
    }
    return result;
}

}  // namespace ss::core
