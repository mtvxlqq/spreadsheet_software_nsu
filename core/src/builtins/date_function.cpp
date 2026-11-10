#include "builtins/date_function.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

#include "core/date.hpp"
#include "core/eval_context.hpp"

namespace ss::core {

std::string_view DateFunction::name() const {
    return "DATE";
}

ValuePtr DateFunction::call(std::span<const Expr* const> args, EvalContext& context) const {
    if (args.size() != 3) {
        return make_error(ErrorCode::Value);
    }
    std::array<int, 3> parts{};
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const ValuePtr value = context.evaluate(*args[i]);
        if (value->is_error()) {
            return value;
        }
        const std::optional<double> number = value->as_number();
        // год, месяц и день должны быть целыми и не слишком большими для int
        if (!number || *number != std::trunc(*number) || std::abs(*number) > max_year) {
            return make_error(ErrorCode::Value);
        }
        parts[i] = static_cast<int>(*number);
    }
    const std::optional<Date> date = date_from_parts(parts[0], parts[1], parts[2]);
    return date ? make_date(*date) : make_error(ErrorCode::Value);
}

}  // namespace ss::core
