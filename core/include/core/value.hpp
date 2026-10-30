#pragma once

#include <compare>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ss::core {

class NumberFormat;

enum class ErrorCode { DIV0, REF, NAME, CYCLE, VALUE };

// Текст ошибки, как его видит пользователь: "#DIV/0!", "#REF!", ...
std::string_view errorText(ErrorCode code);

// Значение ячейки или промежуточный результат формулы.
// Значения неизменяемы, поэтому один объект можно разделять между ячейками
// и потоками без копирования.
// [→ этап 2: DateValue — новый наследник, без правки существующих]
class Value {
public:
    virtual ~Value() = default;
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) = delete;
    Value& operator=(Value&&) = delete;

    // Число для арифметики; nullopt — значение не число (строка, ошибка).
    virtual std::optional<double> asNumber() const = 0;
    virtual std::string toText() const = 0;
    virtual std::string display(const NumberFormat& format) const = 0;

    virtual bool isError() const {
        return false;
    }

    virtual bool isEmpty() const {
        return false;
    }

    // *this <=> other; nullopt — значения несравнимы (будет #VALUE!).
    // Тип other узнаётся двойной диспетчеризацией: compare() вызывает
    // у other нужный compareWith...(), передав себя левым операндом.
    virtual std::optional<std::partial_ordering> compare(const Value& other) const = 0;

    // lhs <=> *this, где lhs — число, строка или пустое значение.
    // По умолчанию несравнимо.
    virtual std::optional<std::partial_ordering> compareWithNumber(double lhs) const;
    virtual std::optional<std::partial_ordering> compareWithString(std::string_view lhs) const;
    virtual std::optional<std::partial_ordering> compareWithEmpty() const;

protected:
    Value() = default;
};

using ValuePtr = std::shared_ptr<const Value>;

// Пустая ячейка. В арифметике это 0, в сравнении со строкой — "".
class EmptyValue final : public Value {
public:
    std::optional<double> asNumber() const override;
    std::string toText() const override;
    std::string display(const NumberFormat& format) const override;

    bool isEmpty() const override {
        return true;
    }

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compareWithNumber(double lhs) const override;
    std::optional<std::partial_ordering> compareWithString(std::string_view lhs) const override;
    std::optional<std::partial_ordering> compareWithEmpty() const override;
};

class NumberValue final : public Value {
public:
    explicit NumberValue(double value) : value_(value) {}

    std::optional<double> asNumber() const override;
    std::string toText() const override;
    std::string display(const NumberFormat& format) const override;

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compareWithNumber(double lhs) const override;
    std::optional<std::partial_ordering> compareWithEmpty() const override;

private:
    double value_;
};

// Строки сравниваются без учёта регистра латинских букв.
class StringValue final : public Value {
public:
    explicit StringValue(std::string text) : text_(std::move(text)) {}

    std::optional<double> asNumber() const override;
    std::string toText() const override;
    std::string display(const NumberFormat& format) const override;

    std::optional<std::partial_ordering> compare(const Value& other) const override;
    std::optional<std::partial_ordering> compareWithString(std::string_view lhs) const override;
    std::optional<std::partial_ordering> compareWithEmpty() const override;

private:
    std::string text_;
};

// Ошибка в формуле — это значение, а не исключение: она распространяется
// по зависимым ячейкам и не ломает остальную таблицу.
class ErrorValue final : public Value {
public:
    explicit ErrorValue(ErrorCode code) : code_(code) {}

    ErrorCode code() const {
        return code_;
    }

    std::optional<double> asNumber() const override;
    std::string toText() const override;
    std::string display(const NumberFormat& format) const override;

    bool isError() const override {
        return true;
    }

    std::optional<std::partial_ordering> compare(const Value& other) const override;

private:
    ErrorCode code_;
};

ValuePtr makeNumber(double value);
ValuePtr makeString(std::string text);
ValuePtr makeError(ErrorCode code);
// Пустое значение одно на всю программу.
ValuePtr emptyValue();

}  // namespace ss::core
