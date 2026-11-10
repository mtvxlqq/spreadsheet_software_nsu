#pragma once

#include <ftxui/dom/elements.hpp>

#include "app/session_view.hpp"

namespace ss::tui {

// Весь экран по текущему состоянию сеанса. Ничего не меняет. Сколько строк
// и столбцов рисовать, берётся из view.viewport(), width нужен для строки формулы.
ftxui::Element render_screen(const ss::app::SessionView& view, int width);

}  // namespace ss::tui
