#include "core/structure_edit.hpp"

#include <algorithm>
#include <map>
#include <optional>

#include "core/address.hpp"
#include "core/errors.hpp"
#include "core/formula_rewrite.hpp"
#include "core/spreadsheet.hpp"

namespace ss::core {

namespace {

using Contents = std::map<CellAddress, CellContent>;

// удалять за краем листа нечего, поэтому лишнее количество обрезаем
StructureEdit checked(StructureEdit edit) {
    const int limit = edit.axis == Axis::Row ? max_rows : max_cols;
    if (edit.at < 0 || edit.at >= limit) {
        throw LimitError("строка или столбец за пределами листа");
    }
    if (edit.count < 1 || edit.count > limit) {
        throw LimitError("столько строк или столбцов на листе нет");
    }
    if (edit.kind == StructureEdit::Kind::Delete) {
        edit.count = std::min(edit.count, limit - edit.at);
    }
    return edit;
}

// всё, что сейчас есть на листе: ячейки с содержимым и пустые со стилем
Contents occupied_cells(const Spreadsheet& document) {
    Contents cells;
    document.for_each_cell(
        [&](CellAddress address) { cells.emplace(address, document.content(address)); });
    document.for_each_style([&](CellAddress address, const CellStyle& /*style*/) {
        cells.try_emplace(address, document.content(address));
    });
    return cells;
}

// то же самое после правки: ячейки на новых местах и с новыми формулами
Contents moved_cells(const Contents& before, const FunctionProvider& functions,
                     const StructureMapping& mapping, StructureEdit::Kind kind) {
    Contents after;
    for (const auto& [address, content] : before) {
        // ячейка едет туда же, куда и ссылка на неё
        const std::optional<CellRef> moved =
            mapping.map_ref({.addr = address, .abs_row = false, .abs_col = false});
        if (moved) {
            after.emplace(moved->addr, CellContent{.raw = rewrite_formula(content.raw, functions, mapping),
                                                   .style = content.style});
        } else if (kind == StructureEdit::Kind::Insert && !content.raw.empty()) {
            // при удалении ячейка пропадает, так и задумано, а вот при вставке
            // она уехала за край листа, и данные молча терять нельзя
            throw LimitError("вставка вытолкнула бы ячейку " + address.to_string() +
                             " за край листа");
        }
    }
    return after;
}

CellContent content_at(const Contents& cells, CellAddress address) {
    const auto it = cells.find(address);
    return it == cells.end() ? CellContent{} : it->second;
}

void add_change(DocumentEdit& edit, CellAddress address, const Contents& before,
                const Contents& after) {
    CellContent old_content = content_at(before, address);
    CellContent new_content = content_at(after, address);
    if (old_content != new_content) {
        edit.cells.push_back({.address = address,
                              .before = std::move(old_content),
                              .after = std::move(new_content)});
    }
}

ColumnWidths moved_widths(const ColumnWidths& widths, const StructureMapping& mapping) {
    ColumnWidths result;
    for (const auto& [col, width] : widths.custom()) {
        const std::optional<CellRef> moved =
            mapping.map_ref({.addr = {.row = 0, .col = col}, .abs_row = false, .abs_col = false});
        if (moved) {
            result.set(moved->addr.col, width);
        }
    }
    return result;
}

}  // namespace

DocumentEdit plan_structure_edit(const Spreadsheet& document, const FunctionProvider& functions,
                                 StructureEdit edit) {
    edit = checked(edit);
    const StructureMapping mapping(edit);
    const Contents before = occupied_cells(document);
    const Contents after = moved_cells(before, functions, mapping, edit.kind);

    DocumentEdit result;
    for (const auto& [address, content] : before) {
        add_change(result, address, before, after);
    }
    for (const auto& [address, content] : after) {
        if (!before.contains(address)) {
            add_change(result, address, before, after);
        }
    }
    if (edit.axis == Axis::Column) {
        ColumnWidths widths = moved_widths(document.column_widths(), mapping);
        if (widths != document.column_widths()) {
            result.widths_before = document.column_widths();
            result.widths_after = std::move(widths);
        }
    }
    return result;
}

}  // namespace ss::core
