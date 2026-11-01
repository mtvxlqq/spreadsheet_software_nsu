#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_map>

#include "core/address.hpp"
#include "core/cell.hpp"
#include "core/value.hpp"

namespace ss::core {

// Хранит только заполненные ячейки. Ничего не пересчитывает,
// этим занимается Recalculator.
class Sheet {
public:
    // nullptr, если ячейка пустая. Указатель живёт до следующего set/remove.
    const Cell* get(CellAddress address) const;

    // адрес за пределами листа -> LimitError
    void set(CellAddress address, Cell cell);
    void remove(CellAddress address) noexcept;

    // Новый результат формулы. Если ячейки нет, ничего не делает.
    void set_value(CellAddress address, ValuePtr value) noexcept;

    std::size_t size() const {
        return cells_.size();
    }

    // самый маленький диапазон со всеми ячейками, nullopt у пустого листа
    std::optional<Range> used_range() const;

    // Обходит ячейки по строкам, в строке слева направо. Порядок нужен,
    // чтобы сохранённый файл не менялся от запуска к запуску.
    void for_each(const std::function<void(CellAddress, const Cell&)>& fn) const;

private:
    std::unordered_map<CellAddress, Cell> cells_;
};

}  // namespace ss::core
