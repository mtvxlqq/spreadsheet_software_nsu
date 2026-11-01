#include "core/builtins.hpp"

#include <memory>

#include "builtins/average_function.hpp"
#include "builtins/if_function.hpp"
#include "builtins/max_function.hpp"
#include "builtins/min_function.hpp"
#include "builtins/sum_function.hpp"
#include "core/function_registry.hpp"

namespace ss::core {

void register_builtins(FunctionRegistry& registry) {
    registry.add("SUM", [] { return std::make_shared<SumFunction>(); });
    registry.add("MIN", [] { return std::make_shared<MinFunction>(); });
    registry.add("MAX", [] { return std::make_shared<MaxFunction>(); });
    registry.add("AVERAGE", [] { return std::make_shared<AverageFunction>(); });
    registry.add("IF", [] { return std::make_shared<IfFunction>(); });
}

}  // namespace ss::core
