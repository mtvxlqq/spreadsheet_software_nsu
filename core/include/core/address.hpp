#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace ss::core {

inline constexpr int kMaxRows = 65536;
inline constexpr int kMaxCols = 702;  // столбцы A..ZZ

// Имя столбца по индексу с нуля: 0 → "A", 25 → "Z", 26 → "AA".
std::string columnName(int col);

// Адрес ячейки: строка и столбец с нуля. Может указывать за пределы листа
// (так хранится ссылка вида A99999999 из формулы) — это проверяет isValid().
struct CellAddress {
    int row = 0;
    int col = 0;

    // Строгий разбор "B12", регистр букв не важен.
    // Неверный текст или адрес за пределами листа → AddressError.
    static CellAddress parse(std::string_view text);

    bool isValid() const;
    std::string toString() const;

    // Порядок: сначала по строке, потом по столбцу.
    auto operator<=>(const CellAddress&) const = default;
};

// Ссылка из формулы. Флаги '$' на этапе 1 только запоминаются,
// они понадобятся при копировании формул.
struct CellRef {
    CellAddress addr;
    bool absRow = false;
    bool absCol = false;

    // "A1", "$A$1", "A$1", "$A1". nullopt — текст не похож на ссылку.
    // Адрес за пределами листа ошибкой разбора не считается: такая ссылка
    // при вычислении даст #REF!.
    static std::optional<CellRef> tryParse(std::string_view text);

    std::string toString() const;

    bool operator==(const CellRef&) const = default;
};

// Прямоугольный диапазон, углы включительно. Всегда нормализован:
// from() — левый верхний угол, to() — правый нижний.
class Range {
public:
    // Углы можно передать в любом порядке.
    Range(CellAddress corner1, CellAddress corner2);

    CellAddress from() const {
        return from_;
    }

    CellAddress to() const {
        return to_;
    }

    bool contains(CellAddress address) const;
    std::size_t size() const;
    // Обход по строкам сверху вниз, в строке — слева направо.
    void forEach(const std::function<void(CellAddress)>& fn) const;
    std::string toString() const;

    bool operator==(const Range&) const = default;

private:
    CellAddress from_;
    CellAddress to_;
};

}  // namespace ss::core

template <>
struct std::hash<ss::core::CellAddress> {
    std::size_t operator()(const ss::core::CellAddress& address) const noexcept {
        // Строка и столбец упаковываются в одно 64-битное число без потерь.
        const std::uint64_t row = static_cast<std::uint32_t>(address.row);
        const std::uint64_t col = static_cast<std::uint32_t>(address.col);
        return std::hash<std::uint64_t>{}((row << 32) | col);
    }
};
