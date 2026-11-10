#pragma once

#include <string>

namespace ss::core {

// Как показывать число в ячейке (паттерн Strategy). Какой формат у ячейки,
// записано в её стиле, а объект по этой записи создаёт NumberFormatRegistry.
class NumberFormat {
public:
    virtual ~NumberFormat() = default;
    NumberFormat(const NumberFormat&) = delete;
    NumberFormat& operator=(const NumberFormat&) = delete;
    NumberFormat(NumberFormat&&) = delete;
    NumberFormat& operator=(NumberFormat&&) = delete;

    virtual std::string format(double value) const = 0;

protected:
    NumberFormat() = default;
};

// до 10 значащих цифр без лишних нулей: 3, 0.1, 0.3333333333, 1e+20
class GeneralFormat final : public NumberFormat {
public:
    std::string format(double value) const override;
};

// ровно digits знаков после точки: 3.14159 при digits = 2 это "3.14"
class FixedFormat final : public NumberFormat {
public:
    explicit FixedFormat(int digits) : digits_(digits) {}

    std::string format(double value) const override;

private:
    int digits_;
};

// доля в процентах: 0.256 при digits = 1 это "25.6%"
class PercentFormat final : public NumberFormat {
public:
    explicit PercentFormat(int digits) : digits_(digits) {}

    std::string format(double value) const override;

private:
    int digits_;
};

}  // namespace ss::core
