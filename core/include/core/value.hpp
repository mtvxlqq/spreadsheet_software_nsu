#pragma once

#include <compare>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ss::core {

class NumberFormat;

enum class ErrorCode { Div0, Ref, Name, Cycle, Value };

// "#DIV/0!", "#REF!" и т.д.
std::string_view error_text(ErrorCode code);

// Значение ячейки или результат вычисления. После создания объект не меняется,
// поэтому его можно спокойно делить через shared_ptr между ячейками и потоками.
// Дату на втором этапе добавим ещё одним наследником.
class Value {
public:
    virtual ~Value() = default;
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) = delete;
    Value& operator=(Value&&) = delete;

    // nullopt, если это не число (строка или ошибка)
    virtual std::optional<double> as_number() const = 0;
    virtual std::string to_text() const = 0;
    virtual std::string display(const NumberFormat& format) const = 0;

    virtual bool is_error() const {
        return false;
    }

    virtual bool is_empty() const {
        return false;
    }

    // *this <=> other, или nullopt, если сравнивать нельзя (число со строкой).
    // Чтобы не делать dynamic_cast, тут двойная диспетчеризация: например,
    // NumberValue::compare вызывает other.compare_with_number(своё число).
    virtual std::optional<std::partial_ordering> compare(const Value& other) const = 0;

    // lhs <=> *this. По умолчанию nullopt, наследник переопределяет то, что умеет.
    virtual std::optional<std::partial_ordering> compare_with_number(double lhs) const;
    virtual std::optional<std::partial_ordering> compare_with_string(std::string_view lhs) const;
    virtual std::optional<std::partial_ordering> compare_with_empty() const;

protected:
    Value() = default;
};

using ValuePtr = std::shared_ptr<const Value>;

// Пустая ячейка. В арифметике это 0, а при сравнении со строкой это "".
class EmptyValue final : public Value {
public:
    std::optional<double> as_number() const override;
    std::string to_text() const override;
    std::string display(const NumberFormat& format) const override;

    bool is_empty() const override {
        return true;
    }

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compare_with_number(double lhs) const override;
    std::optional<std::partial_ordering> compare_with_string(std::string_view lhs) const override;
    std::optional<std::partial_ordering> compare_with_empty() const override;
};

class NumberValue final : public Value {
public:
    explicit NumberValue(double value) : value_(value) {}

    std::optional<double> as_number() const override;
    std::string to_text() const override;
    std::string display(const NumberFormat& format) const override;

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compare_with_number(double lhs) const override;
    std::optional<std::partial_ordering> compare_with_empty() const override;

private:
    double value_;
};

// сравниваются без учёта регистра (только для латиницы)
class StringValue final : public Value {
public:
    explicit StringValue(std::string text) : text_(std::move(text)) {}

    std::optional<double> as_number() const override;
    std::string to_text() const override;
    std::string display(const NumberFormat& format) const override;

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compare_with_string(std::string_view lhs) const override;
    std::optional<std::partial_ordering> compare_with_empty() const override;

private:
    std::string text_;
};

// Ошибку в формуле храним как обычное значение, а не бросаем исключение:
// так она доходит до зависимых ячеек и не ломает остальную таблицу.
class ErrorValue final : public Value {
public:
    explicit ErrorValue(ErrorCode code) : code_(code) {}

    ErrorCode code() const {
        return code_;
    }

    std::optional<double> as_number() const override;
    std::string to_text() const override;
    std::string display(const NumberFormat& format) const override;

    bool is_error() const override {
        return true;
    }

    std::optional<std::partial_ordering> compare(const Value& other) const override;

private:
    ErrorCode code_;
};

ValuePtr make_number(double value);
ValuePtr make_string(std::string text);
ValuePtr make_error(ErrorCode code);
// один общий объект на всю программу
ValuePtr empty_value();

}  // namespace ss::core
