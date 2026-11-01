#include "builtins/sum_function.hpp"

#include "builtins/number_args.hpp"

namespace ss::core {

std::string_view SumFunction::name() const {
    return "SUM";
}

ValuePtr SumFunction::call(std::span<const Expr* const> args, EvalContext& context) const {
    const NumberArgs input = collect_numbers(args, context);
    if (input.error) {
        return input.error;
    }
    double sum = 0;
    for (const double number : input.numbers) {
        sum += number;
    }
    return make_number_or_error(sum);
}

}  // namespace ss::core
