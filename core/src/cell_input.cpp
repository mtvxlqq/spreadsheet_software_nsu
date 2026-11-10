#include "core/cell_input.hpp"

#include <string>

#include "core/parser.hpp"
#include "core/text.hpp"
#include "core/value_parser.hpp"

namespace ss::core {

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
    } else if (ValuePtr value = value_parsers_.parse(text)) {
        cell.value = std::move(value);
    } else {
        cell.value = make_string(std::string(text));
    }
    return cell;
}

}  // namespace ss::core
