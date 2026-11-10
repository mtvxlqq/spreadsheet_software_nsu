#pragma once

namespace ss::tui {

// пять цифр номера строки и отступ
inline constexpr int row_header_width = 6;
// строка формулы, заголовок столбцов и строка состояния
inline constexpr int service_rows = 3;

struct ScreenLayout {
    int visible_rows = 1;
    // сколько позиций экрана остаётся столбцам; сколько их влезет,
    // зависит от ширины каждого, это считает Viewport
    int grid_width = 1;

    bool operator==(const ScreenLayout&) const = default;
};

// Место под сетку в терминале. Хотя бы одна строка и одна позиция,
// даже если окно совсем маленькое.
ScreenLayout compute_layout(int width, int height);

}  // namespace ss::tui
