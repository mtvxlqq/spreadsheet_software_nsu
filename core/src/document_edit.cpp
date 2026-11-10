#include "core/document_edit.hpp"

namespace ss::core {

// в обратном порядке, чтобы отмена шла от последнего изменения к первому
DocumentEdit DocumentEdit::inverse() const {
    DocumentEdit result;
    result.cells.reserve(cells.size());
    for (auto it = cells.rbegin(); it != cells.rend(); ++it) {
        result.cells.push_back({.address = it->address, .before = it->after, .after = it->before});
    }
    return result;
}

}  // namespace ss::core
