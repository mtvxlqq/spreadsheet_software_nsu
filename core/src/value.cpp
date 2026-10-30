#include "core/value.hpp"

#include "core/number_format.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

std::partial_ordering compareIgnoreCase(std::string_view lhs, std::string_view rhs) {
    return toLowerAscii(lhs) <=> toLowerAscii(rhs);
}

}  // namespace

std::string_view errorText(ErrorCode code) {
    switch (code) {
        case ErrorCode::DIV0:
            return "#DIV/0!";
        case ErrorCode::REF:
            return "#REF!";
        case ErrorCode::NAME:
            return "#NAME?";
        case ErrorCode::CYCLE:
            return "#CYCLE!";
        case ErrorCode::VALUE:
            return "#VALUE!";
    }
    return "#VALUE!";  // недостижимо: все коды разобраны выше
}

std::optional<std::partial_ordering> Value::compareWithNumber(double /*lhs*/) const {
    return std::nullopt;
}

std::optional<std::partial_ordering> Value::compareWithString(std::string_view /*lhs*/) const {
    return std::nullopt;
}

std::optional<std::partial_ordering> Value::compareWithEmpty() const {
    return std::nullopt;
}

// --- EmptyValue ---

std::optional<double> EmptyValue::asNumber() const {
    return 0.0;
}

std::string EmptyValue::toText() const {
    return "";
}

std::string EmptyValue::display(const NumberFormat& /*format*/) const {
    return "";
}

std::optional<std::partial_ordering> EmptyValue::compare(const Value& other) const {
    return other.compareWithEmpty();
}

std::optional<std::partial_ordering> EmptyValue::compareWithNumber(double lhs) const {
    return lhs <=> 0.0;
}

std::optional<std::partial_ordering> EmptyValue::compareWithString(std::string_view lhs) const {
    return compareIgnoreCase(lhs, "");
}

std::optional<std::partial_ordering> EmptyValue::compareWithEmpty() const {
    return std::partial_ordering::equivalent;
}

// --- NumberValue ---

std::optional<double> NumberValue::asNumber() const {
    return value_;
}

std::string NumberValue::toText() const {
    return GeneralFormat{}.format(value_);
}

std::string NumberValue::display(const NumberFormat& format) const {
    return format.format(value_);
}

std::optional<std::partial_ordering> NumberValue::compare(const Value& other) const {
    return other.compareWithNumber(value_);
}

std::optional<std::partial_ordering> NumberValue::compareWithNumber(double lhs) const {
    return lhs <=> value_;
}

std::optional<std::partial_ordering> NumberValue::compareWithEmpty() const {
    return 0.0 <=> value_;
}

// --- StringValue ---

std::optional<double> StringValue::asNumber() const {
    return std::nullopt;
}

std::string StringValue::toText() const {
    return text_;
}

std::string StringValue::display(const NumberFormat& /*format*/) const {
    return text_;
}

std::optional<std::partial_ordering> StringValue::compare(const Value& other) const {
    return other.compareWithString(text_);
}

std::optional<std::partial_ordering> StringValue::compareWithString(std::string_view lhs) const {
    return compareIgnoreCase(lhs, text_);
}

std::optional<std::partial_ordering> StringValue::compareWithEmpty() const {
    return compareIgnoreCase("", text_);
}

// --- ErrorValue ---

std::optional<double> ErrorValue::asNumber() const {
    return std::nullopt;
}

std::string ErrorValue::toText() const {
    return std::string(errorText(code_));
}

std::string ErrorValue::display(const NumberFormat& /*format*/) const {
    return toText();
}

std::optional<std::partial_ordering> ErrorValue::compare(const Value& /*other*/) const {
    return std::nullopt;
}

// --- фабрики ---

ValuePtr makeNumber(double value) {
    return std::make_shared<const NumberValue>(value);
}

ValuePtr makeString(std::string text) {
    return std::make_shared<const StringValue>(std::move(text));
}

ValuePtr makeError(ErrorCode code) {
    return std::make_shared<const ErrorValue>(code);
}

ValuePtr emptyValue() {
    // Значение неизменяемо, поэтому один общий экземпляр безопасно
    // разделять между всеми ячейками и потоками.
    static const ValuePtr instance = std::make_shared<const EmptyValue>();
    return instance;
}

}  // namespace ss::core
