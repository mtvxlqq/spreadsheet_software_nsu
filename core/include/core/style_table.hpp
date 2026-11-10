#pragma once

#include <unordered_map>

#include "core/address.hpp"
#include "core/cell_style.hpp"

namespace ss::core {

// Оформление ячеек отдельно от содержимого: очистка ячейки стиль не
// снимает, а пересчёт и used_range про стили не знают.
class StyleTable final {
public:
    // у ячейки без своего стиля стиль по умолчанию
    const CellStyle& get(CellAddress address) const;
    // стиль по умолчанию не храним, чтобы таблица не росла от пустых записей
    void set(CellAddress address, CellStyle style);

private:
    std::unordered_map<CellAddress, CellStyle> styles_;
};

}  // namespace ss::core
