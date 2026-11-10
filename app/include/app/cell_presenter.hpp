#pragma once

#include <string>

#include "core/address.hpp"
#include "core/cell_style.hpp"

namespace ss::core {
class NumberFormatRegistry;
class Spreadsheet;
}  // namespace ss::core

namespace ss::app {

// выравнивание это часть стиля ячейки, а стиль живёт в документе
using Align = ss::core::Align;

// что нарисовать в ячейке таблицы
struct CellView {
    std::string text;
    Align align = Align::Left;
    // число, которое не влезло в столбец, рисуется решётками, а не обрезается
    bool is_number = false;
};

// Как показать ячейку в таблице. Это решает app, чтобы в TUI осталось
// только рисование.
class CellPresenter final {
public:
    // Формат чисел и выравнивание берутся из стиля ячейки. Если выравнивание
    // в стиле не задано, числа прижимаются вправо, остальное влево, как в Excel.
    static CellView present(const ss::core::Spreadsheet& document, ss::core::CellAddress address,
                            const ss::core::NumberFormatRegistry& formats);
};

}  // namespace ss::app
