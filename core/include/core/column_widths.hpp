#pragma once

#include <map>

namespace ss::core {

// ширина столбца в позициях экрана, вместе с пробелом между столбцами
inline constexpr int default_column_width = 10;
inline constexpr int min_column_width = 3;
inline constexpr int max_column_width = 60;

// Ширины столбцов. Хранятся только те, что отличаются от обычной,
// поэтому у нового листа тут пусто.
class ColumnWidths final {
public:
    int get(int col) const;
    // ширина не от min до max или столбец вне листа: LimitError
    void set(int col, int width);

    // столбцы со своей шириной по возрастанию номера
    const std::map<int, int>& custom() const {
        return widths_;
    }

    bool operator==(const ColumnWidths&) const = default;

private:
    std::map<int, int> widths_;
};

}  // namespace ss::core
