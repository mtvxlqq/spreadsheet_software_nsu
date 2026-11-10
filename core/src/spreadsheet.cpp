#include "core/spreadsheet.hpp"

#include <algorithm>
#include <set>
#include <utility>

#include "core/cell.hpp"
#include "core/cell_observer.hpp"
#include "core/formula_printer.hpp"
#include "core/recalculator.hpp"
#include "core/ref_collector.hpp"
#include "sheet_eval_context.hpp"

namespace ss::core {

Spreadsheet::Spreadsheet(const FunctionProvider& functions) : input_parser_(functions) {}

// Сначала всё, что может бросить, пока таблица не тронута: разбор ввода
// и сбор ссылок. Потом граф и лист, и только потом пересчёт.
void Spreadsheet::set_input(CellAddress address, std::string_view raw) {
    PreparedCell prepared = prepare(address, raw);
    if (!prepared.cell) {
        clear(address);
        return;
    }
    store(std::move(prepared));
    recalculate({address});
}

Spreadsheet::PreparedCell Spreadsheet::prepare(CellAddress address, std::string_view raw) const {
    address.check_valid();
    PreparedCell prepared{.address = address, .cell = input_parser_.parse(raw), .dependencies = {}};
    if (prepared.cell && prepared.cell->formula) {
        prepared.dependencies = RefCollector::collect(*prepared.cell->formula);
    }
    return prepared;
}

void Spreadsheet::store(PreparedCell prepared) {
    if (!prepared.cell) {
        graph_.remove_cell(prepared.address);
        sheet_.remove(prepared.address);
        return;
    }
    graph_.set_dependencies(prepared.address, std::move(prepared.dependencies));
    try {
        sheet_.set(prepared.address, std::move(*prepared.cell));
    } catch (...) {
        // Бросить set может только при вставке новой ячейки (если она уже была,
        // это просто перемещение). А у ячейки, которой не было, не было и
        // зависимостей, так что откат это просто remove_cell, он не бросает.
        graph_.remove_cell(prepared.address);
        throw;
    }
}

void Spreadsheet::set_style(CellAddress address, CellStyle style) {
    address.check_valid();
    styles_.set(address, std::move(style));
}

void Spreadsheet::set_column_width(int col, int width) {
    widths_.set(col, width);
}

void Spreadsheet::for_each_style(
    const std::function<void(CellAddress, const CellStyle&)>& fn) const {
    styles_.for_each(fn);
}

const CellStyle& Spreadsheet::style(CellAddress address) const {
    return styles_.get(address);
}

const ColumnWidths& Spreadsheet::column_widths() const {
    return widths_;
}

CellContent Spreadsheet::content(CellAddress address) const {
    return {.raw = raw(address), .style = style(address)};
}

void Spreadsheet::apply(const DocumentEdit& edit) {
    // всё, что может не понравиться пользователю, до первого изменения
    std::vector<PreparedCell> prepared;
    prepared.reserve(edit.cells.size());
    for (const CellChange& change : edit.cells) {
        prepared.push_back(prepare(change.address, change.after.raw));
    }

    std::vector<CellAddress> changed;
    changed.reserve(edit.cells.size());
    // копия тоже может бросить, поэтому делаем её заранее, а меняем через swap
    std::optional<ColumnWidths> widths = edit.widths_after;
    if (widths) {
        std::swap(widths_, *widths);
    }
    std::size_t done = 0;
    try {
        for (; done < prepared.size(); ++done) {
            const CellChange& change = edit.cells[done];
            styles_.set(change.address, change.after.style);
            store(std::move(prepared[done]));
            changed.push_back(change.address);
        }
        recalculate(changed);
    } catch (...) {
        if (widths) {
            std::swap(widths_, *widths);
        }
        // ячейку, на которой упали, тоже возвращаем: стиль у неё мог уже поменяться
        roll_back(edit, std::min(done + 1, edit.cells.size()));
        throw;
    }
}

// Сюда попадаем, только если кончилась память. Каждая ячейка до этого
// ставилась целиком, так что граф и лист согласованы, и остаётся вернуть
// старое содержимое. Не вышло и это: таблица цела, но правка применена частично.
void Spreadsheet::roll_back(const DocumentEdit& edit, std::size_t count) noexcept {
    try {
        std::vector<CellAddress> restored;
        for (std::size_t i = count; i-- > 0;) {
            const CellChange& change = edit.cells[i];
            store(prepare(change.address, change.before.raw));
            styles_.set(change.address, change.before.style);
            restored.push_back(change.address);
        }
        SheetEvalContext context(sheet_);
        Recalculator::run(graph_.plan(restored), sheet_, context);
    } catch (...) {
        // памяти нет даже на откат, сделать тут больше нечего
    }
}

void Spreadsheet::clear(CellAddress address) {
    if (sheet_.get(address) == nullptr) {
        return;  // и так пусто, сообщать не о чем
    }
    graph_.remove_cell(address);
    sheet_.remove(address);
    recalculate({address});
}

ValuePtr Spreadsheet::value(CellAddress address) const {
    const Cell* cell = sheet_.get(address);
    return cell == nullptr ? empty_value() : cell->value;
}

std::string Spreadsheet::raw(CellAddress address) const {
    const Cell* cell = sheet_.get(address);
    return cell == nullptr ? "" : cell->raw;
}

std::string Spreadsheet::formula_text(CellAddress address) const {
    const Cell* cell = sheet_.get(address);
    if (cell == nullptr || !cell->formula) {
        return "";
    }
    return FormulaPrinter::print(*cell->formula);
}

std::optional<Range> Spreadsheet::used_range() const {
    return sheet_.used_range();
}

void Spreadsheet::for_each_cell(const std::function<void(CellAddress)>& fn) const {
    sheet_.for_each([&fn](CellAddress address, const Cell& /*cell*/) { fn(address); });
}

// Граф знает только формулы со ссылками, а =1+2 в нём нет,
// поэтому список формул берём из листа.
void Spreadsheet::recalculate_all() {
    std::vector<CellAddress> formulas;
    sheet_.for_each([&formulas](CellAddress address, const Cell& cell) {
        if (cell.formula) {
            formulas.push_back(address);
        }
    });
    recalculate(formulas);
}

void Spreadsheet::subscribe(CellObserver* observer) {
    if (observer != nullptr && std::ranges::find(observers_, observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void Spreadsheet::unsubscribe(CellObserver* observer) {
    std::erase(observers_, observer);
}

void Spreadsheet::recalculate(const std::vector<CellAddress>& changed) {
    SheetEvalContext context(sheet_);
    notify(Recalculator::run(graph_.plan(changed), sheet_, context));
}

// Наблюдатель может отписаться прямо в on_cells_changed (себя или другого),
// поэтому идём по копии и перед вызовом проверяем, что он ещё подписан.
void Spreadsheet::notify(const std::vector<CellAddress>& cells) {
    if (cells.empty()) {
        return;
    }
    const std::vector<CellObserver*> snapshot = observers_;
    for (CellObserver* observer : snapshot) {
        if (std::ranges::find(observers_, observer) != observers_.end()) {
            observer->on_cells_changed(cells);
        }
    }
}

}  // namespace ss::core
