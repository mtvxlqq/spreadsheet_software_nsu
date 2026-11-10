#include "core/number_format.hpp"

#include <cmath>
#include <format>

namespace ss::core {

namespace {

// Округляем сами, как в школе: 2.5 это 3. std::format округлил бы половину
// к чётному, и 2.5 стало бы 2.
double round_to(double value, int digits) {
    const double scale = std::pow(10.0, digits);
    const double scaled = value * scale;
    // очень большое число при умножении могло уйти в бесконечность
    return std::isfinite(scaled) ? std::round(scaled) / scale : value;
}

// -0.001 с двумя знаками печатается как "-0.00", а минус у нуля ни к чему
std::string fixed_text(double value, int digits) {
    std::string text = std::format("{:.{}f}", round_to(value, digits), digits);
    if (text.starts_with('-') && text.find_first_of("123456789") == std::string::npos) {
        text.erase(0, 1);
    }
    return text;
}

}  // namespace

std::string GeneralFormat::format(double value) const {
    if (value == 0.0) {
        value = 0.0;  // иначе -0.0 напечатается как "-0"
    }
    return std::format("{:.10g}", value);
}

std::string FixedFormat::format(double value) const {
    return fixed_text(value, digits_);
}

std::string PercentFormat::format(double value) const {
    return fixed_text(value * 100, digits_) + "%";
}

}  // namespace ss::core
