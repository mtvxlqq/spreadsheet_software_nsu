#include "core/cell_input.hpp"

#include <charconv>
#include <cmath>
#include <string>
#include <system_error>

#include "core/parser.hpp"

namespace ss::core {

namespace {

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && is_space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

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
    const std::string_view text = trim(input);
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
