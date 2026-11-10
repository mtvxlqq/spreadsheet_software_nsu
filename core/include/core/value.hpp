#pragma once

#include <compare>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "core/date.hpp"

namespace ss::core {

class NumberFormat;

enum class ErrorCode { Div0, Ref, Name, Cycle, Value };

// "#DIV/0!", "#REF!" и т.д.
std::string_view error_text(ErrorCode code);

class Value;
using ValuePtr = std::shared_ptr<const Value>;

// Значение ячейки или результат вычисления. После создания объект не меняется,
// поэтому его можно спокойно делить через shared_ptr между ячейками и потоками.
// Новый тип значения это ещё один наследник: всё, чего он не умеет, уже
// есть здесь с ответом по умолчанию.
class Value {
public:
    virtual ~Value() = default;
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) = delete;
    Value& operator=(Value&&) = delete;

    // nullopt, если это не число (строка или ошибка)
    virtual std::optional<double> as_number() const = 0;
    // nullopt у всего, кроме даты
    virtual std::optional<Date> as_date() const;
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
    virtual std::optional<std::partial_ordering> compare_with_date(Date lhs) const;

    // Своя арифметика типа: *this + other и *this - other. nullptr значит,
    // что ничего особенного нет, и Evaluator посчитает как с числами. Так
    // дата минус дата даёт число дней, а Evaluator про даты не знает.
    virtual ValuePtr plus(const Value& other) const;
    virtual ValuePtr minus(const Value& other) const;

protected:
    Value() = default;
};

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
    std::optional<std::partial_ordering> compare_with_date(Date lhs) const override;
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

// Дата. Для SUM, MIN и MAX это просто число дней (as_number), поэтому они
// работают с датами без правок. Сравнивается только с датой и с пустой
// ячейкой, которая считается раньше любой даты.
class DateValue final : public Value {
public:
    explicit DateValue(Date date) : date_(date) {}

    std::optional<double> as_number() const override;
    std::optional<Date> as_date() const override;
    std::string to_text() const override;
    // формат чисел на дату не действует, она всегда 2024-03-15
    std::string display(const NumberFormat& format) const override;

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compare_with_empty() const override;
    std::optional<std::partial_ordering> compare_with_date(Date lhs) const override;

    // дата плюс число это дата, а две даты складывать нельзя
    ValuePtr plus(const Value& other) const override;
    // дата минус дата это число дней, дата минус число это дата
    ValuePtr minus(const Value& other) const override;

private:
    // дата через days дней или #VALUE!, если вышли за годы 0001..9999
    ValuePtr shifted(double days) const;

    Date date_;
};

ValuePtr make_number(double value);
// для результатов вычислений: inf и nan превращаются в #VALUE!
ValuePtr make_number_or_error(double value);
ValuePtr make_string(std::string text);
ValuePtr make_error(ErrorCode code);
ValuePtr make_date(Date date);
// один общий объект на всю программу
ValuePtr empty_value();

}  // namespace ss::core
