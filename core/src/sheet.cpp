#include "core/sheet.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "core/errors.hpp"

namespace ss::core {

const Cell* Sheet::get(CellAddress address) const {
    const auto it = cells_.find(address);
    return it == cells_.end() ? nullptr : &it->second;
}

void Sheet::set(CellAddress address, Cell cell) {
    if (!address.is_valid()) {
        throw LimitError("ячейка " + address.to_string() + " за пределами листа");
    }
    cells_.insert_or_assign(address, std::move(cell));
}

void Sheet::remove(CellAddress address) noexcept {
    cells_.erase(address);
}

void Sheet::set_value(CellAddress address, ValuePtr value) noexcept {
    const auto it = cells_.find(address);
    if (it != cells_.end()) {
        it->second.value = std::move(value);
    }
}

std::optional<Range> Sheet::used_range() const {
    if (cells_.empty()) {
        return std::nullopt;
    }
    CellAddress top_left = cells_.begin()->first;
    CellAddress bottom_right = top_left;
    for (const auto& [address, cell] : cells_) {
        top_left.row = std::min(top_left.row, address.row);
        top_left.col = std::min(top_left.col, address.col);
        bottom_right.row = std::max(bottom_right.row, address.row);
        bottom_right.col = std::max(bottom_right.col, address.col);
    }
    return Range(top_left, bottom_right);
}

void Sheet::for_each(const std::function<void(CellAddress, const Cell&)>& fn) const {
    // в unordered_map порядка нет, поэтому сначала сортируем адреса
    std::vector<CellAddress> addresses;
    addresses.reserve(cells_.size());
    for (const auto& [address, cell] : cells_) {
        addresses.push_back(address);
    }
    std::sort(addresses.begin(), addresses.end());
    for (const CellAddress address : addresses) {
        fn(address, cells_.at(address));
    }
}

}  // namespace ss::core
