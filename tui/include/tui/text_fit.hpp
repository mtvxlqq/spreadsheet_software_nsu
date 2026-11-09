#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "app/cell_presenter.hpp"

namespace ss::tui {

// Текст ячейки ровно на width экранных позиций. Не влезает: текст обрезаем,
// а число заменяем на решётки, потому что обрезанное число читается как другое.
std::string fit_cell(std::string_view text, int width, ss::app::Align align);

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
