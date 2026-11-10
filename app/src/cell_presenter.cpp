#include "app/cell_presenter.hpp"

#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"

namespace ss::app {

CellView CellPresenter::present(const ss::core::Spreadsheet& document,
                                ss::core::CellAddress address,
                                const ss::core::NumberFormatRegistry& formats) {
    const ss::core::ValuePtr value = document.value(address);
    const ss::core::CellStyle& style = document.style(address);
    // у пустой ячейки as_number() тоже есть (это 0), её отдельно
    const bool is_number = !value->is_empty() && value->as_number().has_value();
    return {.text = value->display(*formats.create(style.format)),
            .align = style.align.value_or(is_number ? Align::Right : Align::Left),
            .is_number = is_number};
}

}  // namespace ss::app
