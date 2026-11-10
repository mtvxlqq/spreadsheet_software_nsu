#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <set>

#include "core/address.hpp"
#include "core/cell.hpp"
#include "core/cell_input.hpp"
#include "core/cell_io.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/dependency_graph.hpp"
#include "core/document_edit.hpp"
#include "core/sheet.hpp"
#include "core/style_table.hpp"
#include "core/value.hpp"

namespace ss::core {

class CellObserver;
class FunctionProvider;
class ValueParserRegistry;

// Главный класс ядра (паттерн Facade). Сам ничего не разбирает и не обходит
// граф, только по очереди зовёт CellInputParser, RefCollector,
// DependencyGraph и Recalculator, а потом сообщает наблюдателям.
class Spreadsheet final : public CellSink, public CellSource {
public:
    // оба реестра должны жить дольше таблицы
    Spreadsheet(const FunctionProvider& functions, const ValueParserRegistry& value_parsers);

    // Если бросит (ParseError, LimitError), таблица останется как была,
    // и наблюдатели ничего не услышат.
    void set_input(CellAddress address, std::string_view raw) override;
    void clear(CellAddress address) override;
    // эти два нужны чтению файла; из программы оформление меняют через apply
    void set_style(CellAddress address, CellStyle style) override;
    void set_column_width(int col, int width) override;

    ValuePtr value(CellAddress address) const override;
    std::string raw(CellAddress address) const override;
    // формула в нашем написании ("=A1+1"), у не-формулы ""
    std::string formula_text(CellAddress address) const;
    std::optional<Range> used_range() const override;
    void for_each_cell(const std::function<void(CellAddress)>& fn) const override;
    void for_each_style(
        const std::function<void(CellAddress, const CellStyle&)>& fn) const override;
    const ColumnWidths& column_widths() const override;

    const CellStyle& style(CellAddress address) const;
    // то, что надо запомнить, чтобы потом вернуть ячейку как была
    CellContent content(CellAddress address) const;

    // Применяет всю правку и один раз пересчитывает и уведомляет. Ошибки ввода
    // (ParseError, LimitError) находятся до первого изменения, тогда таблица
    // остаётся как была. Если посреди кончится память, уже сделанное откатывается.
    void apply(const DocumentEdit& edit);

    // пересчитать все формулы, например после загрузки файла
    void recalculate_all();

    // повторная подписка и отписка неподписанного ничего не делают
    void subscribe(CellObserver* observer);
    void unsubscribe(CellObserver* observer);

private:
    // разобранный ввод одной ячейки; cell пустой, если ячейку надо очистить
    struct PreparedCell {
        CellAddress address;
        std::optional<Cell> cell;
        std::set<CellAddress> dependencies;
    };

    PreparedCell prepare(CellAddress address, std::string_view raw) const;
    // кладёт одну ячейку в граф и лист; если бросит, они друг с другом согласованы
    void store(PreparedCell prepared);
    void roll_back(const DocumentEdit& edit, std::size_t count) noexcept;
    void recalculate(const std::vector<CellAddress>& changed);
    void notify(const std::vector<CellAddress>& cells);

    CellInputParser input_parser_;
    Sheet sheet_;
    DependencyGraph graph_;
    StyleTable styles_;
    ColumnWidths widths_;
    std::vector<CellObserver*> observers_;
};

}  // namespace ss::core
