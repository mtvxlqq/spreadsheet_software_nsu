#pragma once

#include <string>

#include "core/address.hpp"
#include "core/cell_style.hpp"

namespace ss::core {
class NumberFormat;
class Spreadsheet;
}  // namespace ss::core

namespace ss::app {

// выравнивание теперь часть стиля ячейки, а стиль живёт в документе
using Align = ss::core::Align;

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
