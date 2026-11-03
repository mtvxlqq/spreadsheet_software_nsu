#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "core/address.hpp"
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

protected:
    CellSource() = default;
};

}  // namespace ss::core
