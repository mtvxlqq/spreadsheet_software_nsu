#include "core/style_table.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace ss::core {

const CellStyle& StyleTable::get(CellAddress address) const {
    static const CellStyle plain;
    const auto it = styles_.find(address);
    return it == styles_.end() ? plain : it->second;
}

void StyleTable::set(CellAddress address, CellStyle style) {
    if (style == CellStyle{}) {
        styles_.erase(address);
        return;
    }
    styles_.insert_or_assign(address, std::move(style));
}

void StyleTable::for_each(const std::function<void(CellAddress, const CellStyle&)>& fn) const {
    // в unordered_map порядка нет, поэтому сначала сортируем адреса
    std::vector<CellAddress> addresses;
    addresses.reserve(styles_.size());
    for (const auto& [address, style] : styles_) {
        addresses.push_back(address);
    }
    std::ranges::sort(addresses);
    for (const CellAddress address : addresses) {
        fn(address, styles_.at(address));
    }
}

}  // namespace ss::core
