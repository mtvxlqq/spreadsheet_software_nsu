#include "core/number_format.hpp"

#include <format>

namespace ss::core {

std::string GeneralFormat::format(double value) const {
    if (value == 0.0) {
        value = 0.0;  // -0 == 0, и показывать его надо как "0", а не "-0"
    }
    return std::format("{:.10g}", value);
}

}  // namespace ss::core
