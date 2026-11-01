#include "builtins/average_function.hpp"

#include "builtins/number_args.hpp"

namespace ss::core {

std::string_view AverageFunction::name() const {
    return "AVERAGE";
}

ValuePtr AverageFunction::call(std::span<const Expr* const> args, EvalContext& context) const {
    const NumberArgs input = collect_numbers(args, context);
    if (input.error) {
        return input.error;
    }
    if (input.numbers.empty()) {
        return make_error(ErrorCode::Div0);
    }
    double sum = 0;
    for (const double number : input.numbers) {
        sum += number;
    }
    return make_number_or_error(sum / static_cast<double>(input.numbers.size()));
}

}  // namespace ss::core
