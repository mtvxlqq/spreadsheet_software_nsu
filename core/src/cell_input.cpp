#include "core/cell_input.hpp"

#include <charconv>
#include <cmath>
#include <string>
#include <system_error>

#include "core/parser.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

// from_chars понимает ещё "inf" и "nan", их считаем текстом
std::optional<double> read_number(std::string_view text) {
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

}  // namespace

std::optional<Cell> CellInputParser::parse(std::string_view input) const {
    const std::string_view text = trim_spaces(input);
    if (text.empty()) {
        return std::nullopt;
    }
    Cell cell{.raw = std::string(text), .formula = nullptr, .value = nullptr};
    if (text.front() == '=') {
        cell.formula = Parser::parse(text, functions_);
        cell.value = empty_value();
    } else if (text.front() == '\'') {
        cell.value = make_string(std::string(text.substr(1)));
    } else if (const std::optional<double> number = read_number(text)) {
        cell.value = make_number(*number);
    } else {
        cell.value = make_string(std::string(text));
    }
    return cell;
}

}  // namespace ss::core
