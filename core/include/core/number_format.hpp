#pragma once

#include <string>

namespace ss::core {

// Способ показать число в ячейке (Strategy).
// [→ этап 2: FixedFormat, PercentFormat — новые наследники]
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

// До 10 значащих цифр без хвостовых нулей: 3, 0.1, 0.3333333333, 1e+20.
class GeneralFormat final : public NumberFormat {
public:
    std::string format(double value) const override;
};

}  // namespace ss::core
