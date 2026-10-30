#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace ss::core {

inline constexpr int max_rows = 65536;
inline constexpr int max_cols = 702;  // A..ZZ

// 0 -> "A", 25 -> "Z", 26 -> "AA"
std::string column_name(int col);

// Строка и столбец считаются с нуля. Адрес может быть и за пределами листа
// (например, ссылка A99999999 из формулы), для этого есть is_valid().
struct CellAddress {
    int row = 0;
    int col = 0;

    // "B12" или "b12"; если адрес кривой или вне листа, бросает AddressError
    static CellAddress parse(std::string_view text);

    bool is_valid() const;
    std::string to_string() const;

    // сначала по строке, потом по столбцу
    auto operator<=>(const CellAddress&) const = default;
};

// Ссылка из формулы. Флаги $ пока только запоминаем, понадобятся
// на втором этапе при копировании формул.
struct CellRef {
    CellAddress addr;
    bool abs_row = false;
    bool abs_col = false;

    // Понимает A1, $A$1, A$1, $A1. Адрес вне листа тут не ошибка:
    // такая ссылка потом просто посчитается в #REF!
    static std::optional<CellRef> try_parse(std::string_view text);

    std::string to_string() const;

    bool operator==(const CellRef&) const = default;
};

// Диапазон вроде A1:B3, углы входят. from() всегда левый верхний угол,
// to() правый нижний. Поля закрыты, чтобы этот порядок нельзя было сломать.
class Range {
public:
    // углы можно передать в любом порядке, B2:A1 тоже подойдёт
    Range(CellAddress corner1, CellAddress corner2);

    CellAddress from() const {
        return from_;
    }

    CellAddress to() const {
        return to_;
    }

    bool contains(CellAddress address) const;
    std::size_t size() const;
    // по строкам, в каждой слева направо
    void for_each(const std::function<void(CellAddress)>& fn) const;
    std::string to_string() const;

    bool operator==(const Range&) const = default;

private:
    CellAddress from_;
    CellAddress to_;
};

}  // namespace ss::core

template <>
struct std::hash<ss::core::CellAddress> {
    std::size_t operator()(const ss::core::CellAddress& address) const noexcept {
        // row в старшие 32 бита, col в младшие, так разные адреса не совпадут
        const std::uint64_t row = static_cast<std::uint32_t>(address.row);
        const std::uint64_t col = static_cast<std::uint32_t>(address.col);
        return std::hash<std::uint64_t>{}((row << 32) | col);
    }
};
