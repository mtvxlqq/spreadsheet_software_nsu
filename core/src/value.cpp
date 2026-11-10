#include "core/value.hpp"

#include <cmath>

#include "core/number_format.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

std::partial_ordering compare_ignore_case(std::string_view lhs, std::string_view rhs) {
    return to_lower_ascii(lhs) <=> to_lower_ascii(rhs);
}

}  // namespace

std::string_view error_text(ErrorCode code) {
    switch (code) {
        case ErrorCode::Div0:
            return "#DIV/0!";
        case ErrorCode::Ref:
            return "#REF!";
        case ErrorCode::Name:
            return "#NAME?";
        case ErrorCode::Cycle:
            return "#CYCLE!";
        case ErrorCode::Value:
            return "#VALUE!";
    }
    return "#VALUE!";  // сюда не попадём, но без return gcc ругается
}

std::optional<std::partial_ordering> Value::compare_with_number(double /*lhs*/) const {
    return std::nullopt;
}

std::optional<std::partial_ordering> Value::compare_with_string(std::string_view /*lhs*/) const {
    return std::nullopt;
}

std::optional<std::partial_ordering> Value::compare_with_empty() const {
    return std::nullopt;
}

std::optional<std::partial_ordering> Value::compare_with_date(Date /*lhs*/) const {
    return std::nullopt;
}

std::optional<Date> Value::as_date() const {
    return std::nullopt;
}

ValuePtr Value::plus(const Value& /*other*/) const {
    return nullptr;
}

ValuePtr Value::minus(const Value& /*other*/) const {
    return nullptr;
}

std::optional<double> EmptyValue::as_number() const {
    return 0.0;
}

std::string EmptyValue::to_text() const {
    return "";
}

std::string EmptyValue::display(const NumberFormat& /*format*/) const {
    return "";
}

std::optional<std::partial_ordering> EmptyValue::compare(const Value& other) const {
    return other.compare_with_empty();
}

std::optional<std::partial_ordering> EmptyValue::compare_with_number(double lhs) const {
    return lhs <=> 0.0;
}

std::optional<std::partial_ordering> EmptyValue::compare_with_string(std::string_view lhs) const {
    return compare_ignore_case(lhs, "");
}

std::optional<std::partial_ordering> EmptyValue::compare_with_empty() const {
    return std::partial_ordering::equivalent;
}

// слева дата, справа пусто: любая дата позже пустой ячейки
std::optional<std::partial_ordering> EmptyValue::compare_with_date(Date /*lhs*/) const {
    return std::partial_ordering::greater;
}

std::optional<double> NumberValue::as_number() const {
    return value_;
}

std::string NumberValue::to_text() const {
    return GeneralFormat{}.format(value_);
}

std::string NumberValue::display(const NumberFormat& format) const {
    return format.format(value_);
}

std::optional<std::partial_ordering> NumberValue::compare(const Value& other) const {
    return other.compare_with_number(value_);
}

std::optional<std::partial_ordering> NumberValue::compare_with_number(double lhs) const {
    return lhs <=> value_;
}

std::optional<std::partial_ordering> NumberValue::compare_with_empty() const {
    return 0.0 <=> value_;
}

std::optional<double> StringValue::as_number() const {
    return std::nullopt;
}

std::string StringValue::to_text() const {
    return text_;
}

std::string StringValue::display(const NumberFormat& /*format*/) const {
    return text_;
}

std::optional<std::partial_ordering> StringValue::compare(const Value& other) const {
    return other.compare_with_string(text_);
}

std::optional<std::partial_ordering> StringValue::compare_with_string(std::string_view lhs) const {
    return compare_ignore_case(lhs, text_);
}

std::optional<std::partial_ordering> StringValue::compare_with_empty() const {
    return compare_ignore_case("", text_);
}

std::optional<double> ErrorValue::as_number() const {
    return std::nullopt;
}

std::string ErrorValue::to_text() const {
    return std::string(error_text(code_));
}

std::string ErrorValue::display(const NumberFormat& /*format*/) const {
    return to_text();
}

std::optional<std::partial_ordering> ErrorValue::compare(const Value& /*other*/) const {
    return std::nullopt;
}

std::optional<double> DateValue::as_number() const {
    return static_cast<double>(date_.time_since_epoch().count());
}

std::optional<Date> DateValue::as_date() const {
    return date_;
}

std::string DateValue::to_text() const {
    return format_date(date_);
}

std::string DateValue::display(const NumberFormat& /*format*/) const {
    return to_text();
}

std::optional<std::partial_ordering> DateValue::compare(const Value& other) const {
    return other.compare_with_date(date_);
}

// слева пусто, справа дата
std::optional<std::partial_ordering> DateValue::compare_with_empty() const {
    return std::partial_ordering::less;
}

std::optional<std::partial_ordering> DateValue::compare_with_date(Date lhs) const {
    return lhs <=> date_;
}

ValuePtr DateValue::plus(const Value& other) const {
    if (other.as_date()) {
        return make_error(ErrorCode::Value);
    }
    const std::optional<double> days = other.as_number();
    // со строкой ничего особенного не делаем, Evaluator сам даст #VALUE!
    return days ? shifted(*days) : nullptr;
}

ValuePtr DateValue::minus(const Value& other) const {
    if (const std::optional<Date> date = other.as_date()) {
        return make_number(static_cast<double>((date_ - *date).count()));
    }
    const std::optional<double> days = other.as_number();
    return days ? shifted(-*days) : nullptr;
}

ValuePtr DateValue::shifted(double days) const {
    // слишком большое число нельзя даже превратить в целое
    constexpr double too_many = 1e9;
    if (!std::isfinite(days) || std::abs(days) > too_many) {
        return make_error(ErrorCode::Value);
    }
    // времени в датах нет, поэтому дробную часть отбрасываем
    const std::optional<Date> moved = shift_date(date_, static_cast<long long>(std::trunc(days)));
    return moved ? make_date(*moved) : make_error(ErrorCode::Value);
}

ValuePtr make_number(double value) {
    return std::make_shared<const NumberValue>(value);
}

ValuePtr make_number_or_error(double value) {
    if (!std::isfinite(value)) {
        return make_error(ErrorCode::Value);
    }
    return make_number(value);
}

ValuePtr make_string(std::string text) {
    return std::make_shared<const StringValue>(std::move(text));
}

ValuePtr make_error(ErrorCode code) {
    return std::make_shared<const ErrorValue>(code);
}

ValuePtr make_date(Date date) {
    return std::make_shared<const DateValue>(date);
}

ValuePtr empty_value() {
    // пустое значение не меняется, так что хватит одного объекта на всех
    static const ValuePtr instance = std::make_shared<const EmptyValue>();
    return instance;
}

}  // namespace ss::core
