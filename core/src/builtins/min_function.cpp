#include "builtins/min_function.hpp"

#include "builtins/number_args.hpp"

namespace ss::core {

std::string_view MinFunction::name() const {
    return "MIN";
}

ValuePtr MinFunction::call(std::span<const Expr* const> args, EvalContext& context) const {
    const NumberArgs input = collect_numbers(args, context);
    if (input.error) {
        return input.error;
    }
    if (input.numbers.empty()) {
        return make_number(0);
    }
    double result = input.numbers.front();
    for (const double number : input.numbers) {
        if (number < result) {
            result = number;
        }
    }
    return make_number(result);
}

}  // namespace ss::core
