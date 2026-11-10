#include "core/style_table.hpp"

#include <utility>

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

}  // namespace ss::core
