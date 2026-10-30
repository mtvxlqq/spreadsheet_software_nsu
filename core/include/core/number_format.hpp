#pragma once

#include <string>

namespace ss::core {

// Как показывать число в ячейке (паттерн Strategy).
// На втором этапе сюда добавятся FixedFormat и PercentFormat.
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

}  // namespace ss::core
