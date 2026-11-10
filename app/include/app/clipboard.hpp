#pragma once

#include <vector>

#include "core/address.hpp"
#include "core/document_edit.hpp"

namespace ss::app {

// Скопированные ячейки. Тут лежат копии содержимого, а не ссылки на ячейки,
// поэтому после копирования исходные ячейки можно менять и удалять.
struct Clipboard {
    struct Item {
        // где ячейка была, когда её копировали
        ss::core::CellAddress address;
        ss::core::CellContent content;
    };

    // весь скопированный диапазон вместе с пустыми ячейками
    ss::core::Range source;
    // только заполненные ячейки
    std::vector<Item> items;
};

}  // namespace ss::app
