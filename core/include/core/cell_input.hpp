#pragma once

#include <optional>
#include <string_view>

#include "core/cell.hpp"

namespace ss::core {

class FunctionProvider;

// Превращает то, что пользователь ввёл в ячейку, в её содержимое.
// Сам лист не трогает.
//
// Пробелы по краям ввода всегда обрезаются, так что " 12 " это число 12.
// Дальше смотрим на первый символ:
//   ""      -> nullopt, ячейку надо очистить
//   "=1+A1" -> формула (на ошибку в ней бросает ParseError)
//   "'12"   -> текст "12", апостроф нужен, чтобы ввести число как текст
//   "12"    -> число, если вся строка читается как число
//   иначе   -> текст, например "12abc"
class CellInputParser {
public:
    explicit CellInputParser(const FunctionProvider& functions) : functions_(functions) {}

    // У формулы value пока пустое, посчитает его Recalculator.
    std::optional<Cell> parse(std::string_view input) const;

private:
    const FunctionProvider& functions_;
};

}  // namespace ss::core
