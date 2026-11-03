#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/address.hpp"
#include "core/cell_input.hpp"
#include "core/cell_io.hpp"
#include "core/dependency_graph.hpp"
#include "core/sheet.hpp"
#include "core/value.hpp"

namespace ss::core {

class CellObserver;
class FunctionProvider;

// Главный класс ядра (паттерн Facade). Сам ничего не разбирает и не обходит
// граф, только по очереди зовёт CellInputParser, RefCollector,
// DependencyGraph и Recalculator, а потом сообщает наблюдателям.
class Spreadsheet final : public CellSink, public CellSource {
public:
    // functions должен жить дольше таблицы
    explicit Spreadsheet(const FunctionProvider& functions);

    // Строгая гарантия: если бросит (ParseError, LimitError), таблица
    // и наблюдатели ничего не заметят.
    void set_input(CellAddress address, std::string_view raw) override;
    void clear(CellAddress address) override;

    ValuePtr value(CellAddress address) const override;
    std::string raw(CellAddress address) const override;
    // формула в нашем написании ("=A1+1"), у не-формулы ""
    std::string formula_text(CellAddress address) const;
    std::optional<Range> used_range() const override;
    void for_each_cell(const std::function<void(CellAddress)>& fn) const override;

    // пересчитать все формулы, например после загрузки файла
    void recalculate_all();

    // повторная подписка и отписка неподписанного ничего не делают
    void subscribe(CellObserver* observer);
    void unsubscribe(CellObserver* observer);

private:
    void recalculate(const std::vector<CellAddress>& changed);
    void notify(const std::vector<CellAddress>& cells);

    CellInputParser input_parser_;
    Sheet sheet_;
    DependencyGraph graph_;
    std::vector<CellObserver*> observers_;
};

}  // namespace ss::core
