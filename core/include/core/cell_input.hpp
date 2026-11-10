#pragma once

#include <optional>
#include <string_view>

#include "core/cell.hpp"

namespace ss::core {

class FunctionProvider;
class ValueParserRegistry;

// Превращает то, что пользователь ввёл в ячейку, в её содержимое.
// Сам лист не трогает.
//
// Пробелы по краям ввода всегда обрезаются, так что " 12 " это число 12.
// Дальше смотрим на первый символ:
//   ""      пусто, вернёт nullopt, и ячейку надо очистить
//   "=1+A1" формула (если в ней ошибка, бросит ParseError)
//   "'12"   текст "12": апостроф нужен, чтобы ввести число как текст
//   иначе   спрашиваем разборщики значений: число, дата и т.д.
//           Никто не узнал, значит это просто текст, например "12abc"
class CellInputParser {
public:
    // оба реестра должны жить дольше разборщика
    CellInputParser(const FunctionProvider& functions, const ValueParserRegistry& value_parsers)
        : functions_(functions), value_parsers_(value_parsers) {}

    // У формулы value пока пустое, посчитает его Recalculator.
    std::optional<Cell> parse(std::string_view input) const;

private:
    const FunctionProvider& functions_;
    const ValueParserRegistry& value_parsers_;
};

}  // namespace ss::core
