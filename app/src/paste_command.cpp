#include "app/paste_command.hpp"

#include <set>
#include <utility>

#include "core/errors.hpp"
#include "core/formula_rewrite.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

using ss::core::CellAddress;
using ss::core::CellContent;

PasteCommand::PasteCommand(Clipboard clipboard, CellAddress target,
                           const ss::core::FunctionProvider& functions)
    : clipboard_(std::move(clipboard)), target_(target), functions_(functions) {}

ss::core::DocumentEdit PasteCommand::plan(const ss::core::Spreadsheet& document) const {
    const int d_row = target_.row - clipboard_.source.from().row;
    const int d_col = target_.col - clipboard_.source.from().col;
    const CellAddress last{.row = clipboard_.source.to().row + d_row,
                           .col = clipboard_.source.to().col + d_col};
    if (!target_.is_valid() || !last.is_valid()) {
        throw ss::core::LimitError("вставка не помещается в лист");
    }

    ss::core::DocumentEdit edit;
    std::set<CellAddress> written;
    for (const Clipboard::Item& item : clipboard_.items) {
        const CellAddress address{.row = item.address.row + d_row, .col = item.address.col + d_col};
        CellContent after{
            .raw = ss::core::translate_formula(item.content.raw, functions_, d_row, d_col),
            .style = item.content.style};
        edit.cells.push_back(
            {.address = address, .before = document.content(address), .after = std::move(after)});
        written.insert(address);
    }
    // Обходим заполненные ячейки листа, а не весь диапазон: он бывает огромным.
    const ss::core::Range area(target_, last);
    document.for_each_cell([&](CellAddress address) {
        if (area.contains(address) && !written.contains(address)) {
            const CellContent before = document.content(address);
            edit.cells.push_back(
                {.address = address, .before = before, .after = {.raw = "", .style = before.style}});
        }
    });
    return edit;
}

}  // namespace ss::app
