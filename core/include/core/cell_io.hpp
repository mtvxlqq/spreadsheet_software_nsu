#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "core/address.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/value.hpp"

namespace ss::core {

// Куда чтение файла складывает ячейки. Весь Spreadsheet формату не нужен.
class CellSink {
public:
    virtual ~CellSink() = default;
    CellSink(const CellSink&) = delete;
    CellSink& operator=(const CellSink&) = delete;
    CellSink(CellSink&&) = delete;
    CellSink& operator=(CellSink&&) = delete;

    // как будто пользователь ввёл raw в ячейку
    virtual void set_input(CellAddress address, std::string_view raw) = 0;
    virtual void clear(CellAddress address) = 0;
    // оформление из файла; стиль можно дать и пустой ячейке
    virtual void set_style(CellAddress address, CellStyle style) = 0;
    // недопустимая ширина даёт LimitError
    virtual void set_column_width(int col, int width) = 0;

protected:
    CellSink() = default;
};

// Откуда запись файла берёт ячейки.
class CellSource {
public:
    virtual ~CellSource() = default;
    CellSource(const CellSource&) = delete;
    CellSource& operator=(const CellSource&) = delete;
    CellSource(CellSource&&) = delete;
    CellSource& operator=(CellSource&&) = delete;

    // nullopt, если лист пустой
    virtual std::optional<Range> used_range() const = 0;
    // у пустой ячейки "" и EmptyValue
    virtual std::string raw(CellAddress address) const = 0;
    virtual ValuePtr value(CellAddress address) const = 0;
    // заполненные ячейки по строкам, в строке слева направо
    virtual void for_each_cell(const std::function<void(CellAddress)>& fn) const = 0;
    // Ячейки со своим оформлением в том же порядке. Среди них бывают пустые.
    virtual void for_each_style(
        const std::function<void(CellAddress, const CellStyle&)>& fn) const = 0;
    virtual const ColumnWidths& column_widths() const = 0;

protected:
    CellSource() = default;
};

}  // namespace ss::core
