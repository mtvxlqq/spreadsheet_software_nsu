#include "core/value_parser.hpp"

#include <charconv>
#include <cmath>
#include <optional>
#include <system_error>
#include <utility>

#include "core/date.hpp"

namespace ss::core {

namespace {

// "12", "-0.5", "1e3"; вся строка должна читаться как число
class NumberParser final : public ValueParser {
public:
    ValuePtr parse(std::string_view text) const override {
        double value = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        // from_chars понимает ещё "inf" и "nan", их считаем текстом
        if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value)) {
            return nullptr;
        }
        return make_number(value);
    }
};

// "2024-03-15" или "15.03.2024"; несуществующая дата остаётся текстом
class DateParser final : public ValueParser {
public:
    ValuePtr parse(std::string_view text) const override {
        const std::optional<Date> date = parse_date(text);
        return date ? make_date(*date) : nullptr;
    }
};

}  // namespace

void ValueParserRegistry::add(std::shared_ptr<const ValueParser> parser) {
    parsers_.push_back(std::move(parser));
}

ValuePtr ValueParserRegistry::parse(std::string_view text) const {
    for (const std::shared_ptr<const ValueParser>& parser : parsers_) {
        if (ValuePtr value = parser->parse(text)) {
            return value;
        }
    }
    return nullptr;
}

void register_value_parsers(ValueParserRegistry& registry) {
    registry.add(std::make_shared<const NumberParser>());
    registry.add(std::make_shared<const DateParser>());
}

}  // namespace ss::core
