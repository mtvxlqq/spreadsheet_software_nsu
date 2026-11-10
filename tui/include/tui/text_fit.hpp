#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "app/cell_presenter.hpp"

namespace ss::tui {

// что делать с текстом, который не влез
enum class Overflow {
    Cut,     // обрезать, так поступаем с текстом
    Hashes,  // заменить решётками: обрезанное число читалось бы как другое
};

// Текст ячейки ровно на width экранных позиций.
std::string fit_cell(std::string_view text, int width, ss::app::Align align,
                     Overflow overflow = Overflow::Cut);

// Кусок строки правки, который виден на экране. at это символ под кареткой,
// его рисуют выделенным; в конце строки это пробел.
struct CaretWindow {
    std::string before;
    std::string at;
    std::string after;
};

// caret в байтах, как в SessionView::edit_caret. Вместе куски занимают
// не больше width позиций, и каретка всегда попадает в окно.
CaretWindow caret_window(std::string_view text, std::size_t caret, int width);

}  // namespace ss::tui
