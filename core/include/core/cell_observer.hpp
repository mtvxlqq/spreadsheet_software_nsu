#pragma once

#include <span>

#include "core/address.hpp"

namespace ss::core {

// Кому интересно, что ячейки поменялись (паттерн Observer). Подписчик сам
// обязан отписаться до своего уничтожения: Spreadsheet хранит голые указатели.
class CellObserver {
public:
    virtual ~CellObserver() = default;
    CellObserver(const CellObserver&) = delete;
    CellObserver& operator=(const CellObserver&) = delete;
    CellObserver(CellObserver&&) = delete;
    CellObserver& operator=(CellObserver&&) = delete;

    // все ячейки, которые изменились после одной операции, разом
    virtual void on_cells_changed(std::span<const CellAddress> cells) = 0;

protected:
    CellObserver() = default;
};

}  // namespace ss::core
