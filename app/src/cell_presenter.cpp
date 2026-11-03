#include "app/cell_presenter.hpp"

#include "core/spreadsheet.hpp"
#include "core/value.hpp"

namespace ss::app {

std::string CellPresenter::text(const ss::core::Spreadsheet& document,
                                ss::core::CellAddress address,
                                const ss::core::NumberFormat& format) {
    return document.value(address)->display(format);
}

Align CellPresenter::align(const ss::core::Spreadsheet& document, ss::core::CellAddress address) {
    const ss::core::ValuePtr value = document.value(address);
    // у пустой ячейки as_number() тоже есть (это 0), её отдельно
    if (!value->is_empty() && value->as_number().has_value()) {
        return Align::Right;
    }
    return Align::Left;
}

}  // namespace ss::app
