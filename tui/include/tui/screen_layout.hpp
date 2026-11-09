#pragma once

namespace ss::tui {

// пять цифр номера строки и отступ
inline constexpr int row_header_width = 6;
// ширина пока у всех столбцов одна, как и в Viewport
inline constexpr int column_width = 10;
// строка формулы, заголовок столбцов и строка состояния
inline constexpr int service_rows = 3;

struct ScreenLayout {
    int visible_rows = 1;
    int visible_cols = 1;

    bool operator==(const ScreenLayout&) const = default;
};

// Сколько строк и столбцов сетки влезает в терминал. Хотя бы по одной,
// даже если окно совсем маленькое.
ScreenLayout compute_layout(int width, int height);

}  // namespace ss::tui
