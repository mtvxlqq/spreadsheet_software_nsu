#pragma once

#include <string>

#include "core/address.hpp"

namespace ss::core {
class NumberFormat;
class Spreadsheet;
}  // namespace ss::core

namespace ss::app {

enum class Align { Left, Right };

// Как показать ячейку в таблице. Это решает app, чтобы в TUI осталось
// только рисование.
class CellPresenter final {
public:
    static std::string text(const ss::core::Spreadsheet& document, ss::core::CellAddress address,
                            const ss::core::NumberFormat& format);
    // числа прижимаем вправо, всё остальное влево, как в Excel
    static Align align(const ss::core::Spreadsheet& document, ss::core::CellAddress address);
};

}  // namespace ss::app
