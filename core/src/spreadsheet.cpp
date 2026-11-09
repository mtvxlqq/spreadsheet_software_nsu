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
    address.check_valid();
    std::optional<Cell> cell = input_parser_.parse(raw);
    if (!cell) {
        clear(address);
        return;
    }
    std::set<CellAddress> dependencies;
    if (cell->formula) {
        dependencies = RefCollector::collect(*cell->formula);
    }

    graph_.set_dependencies(address, std::move(dependencies));
    try {
        sheet_.set(address, std::move(*cell));
    } catch (...) {
        // Бросить set может только при вставке новой ячейки (если она уже была,
        // это просто перемещение). А у ячейки, которой не было, не было и
        // зависимостей, так что откат это просто remove_cell, он не бросает.
        graph_.remove_cell(address);
        throw;
    }
    recalculate({address});
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
