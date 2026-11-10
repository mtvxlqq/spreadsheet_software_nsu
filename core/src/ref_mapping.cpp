#include "core/ref_mapping.hpp"

#include <algorithm>

namespace ss::core {

std::optional<CellRef> CopyMapping::map_ref(CellRef ref) const {
    if (!ref.abs_row) {
        ref.addr.row += d_row_;
    }
    if (!ref.abs_col) {
        ref.addr.col += d_col_;
    }
    if (!ref.addr.is_valid()) {
        return std::nullopt;
    }
    return ref;
}

std::optional<std::pair<CellRef, CellRef>> CopyMapping::map_range(CellRef from, CellRef to) const {
    const std::optional<CellRef> new_from = map_ref(from);
    const std::optional<CellRef> new_to = map_ref(to);
    if (!new_from || !new_to) {
        return std::nullopt;
    }
    return std::pair{*new_from, *new_to};
}

int StructureMapping::position_of(CellAddress address) const {
    return edit_.axis == Axis::Row ? address.row : address.col;
}

void StructureMapping::set_position(CellAddress& address, int position) const {
    (edit_.axis == Axis::Row ? address.row : address.col) = position;
}

int StructureMapping::limit() const {
    return edit_.axis == Axis::Row ? max_rows : max_cols;
}

std::optional<CellRef> StructureMapping::map_ref(CellRef ref) const {
    // ссылка за листом и так даёт #REF!, пусть остаётся как написана
    if (!ref.addr.is_valid()) {
        return ref;
    }
    // ссылка на одну ячейку это диапазон из одной позиции
    const int position = position_of(ref.addr);
    const std::optional<std::pair<int, int>> moved = map_span(position, position);
    // вставка могла упереть диапазон в край листа, но одну ячейку обрезать нельзя
    if (!moved || (edit_.kind == StructureEdit::Kind::Insert && position >= edit_.at &&
                   position + edit_.count >= limit())) {
        return std::nullopt;
    }
    set_position(ref.addr, moved->first);
    return ref;
}

std::optional<std::pair<CellRef, CellRef>> StructureMapping::map_range(CellRef from,
                                                                       CellRef to) const {
    if (!from.addr.is_valid() || !to.addr.is_valid()) {
        return std::pair{from, to};
    }
    // углы могли написать в любом порядке, B2:A1 тоже диапазон
    const int from_position = position_of(from.addr);
    const int to_position = position_of(to.addr);
    const std::optional<std::pair<int, int>> moved =
        map_span(std::min(from_position, to_position), std::max(from_position, to_position));
    if (!moved) {
        return std::nullopt;
    }
    set_position(from.addr, from_position <= to_position ? moved->first : moved->second);
    set_position(to.addr, from_position <= to_position ? moved->second : moved->first);
    return std::pair{from, to};
}

// Куда попадает отрезок [low, high]. Правила как в Excel: вставка внутрь его
// растягивает, удаление края сжимает, удаление целиком даёт nullopt.
std::optional<std::pair<int, int>> StructureMapping::map_span(int low, int high) const {
    const int at = edit_.at;
    const int count = edit_.count;
    if (edit_.kind == StructureEdit::Kind::Insert) {
        const int new_low = low < at ? low : low + count;
        if (new_low >= limit()) {
            return std::nullopt;
        }
        return std::pair{new_low, std::min(high < at ? high : high + count, limit() - 1)};
    }
    const int last_deleted = at + count - 1;
    if (low >= at && high <= last_deleted) {
        return std::nullopt;
    }
    const int new_low = low < at ? low : (low <= last_deleted ? at : low - count);
    const int new_high = high < at ? high : (high <= last_deleted ? at - 1 : high - count);
    return std::pair{new_low, new_high};
}

}  // namespace ss::core
