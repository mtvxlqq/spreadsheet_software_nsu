#pragma once

#include <string>
#include <string_view>

namespace ss::core {

class FunctionProvider;
class RefMapping;

// Содержимое ячейки после сдвига ссылок. Не формула возвращается как есть.
// Формула, в которой ни одна ссылка не изменилась, тоже: незачем менять
// то, как её написал пользователь. На кривой формуле бросает ParseError.
std::string rewrite_formula(std::string_view raw, const FunctionProvider& functions,
                            const RefMapping& mapping);

// то, что окажется в ячейке, если raw скопировать на d_row строк и d_col столбцов
std::string translate_formula(std::string_view raw, const FunctionProvider& functions, int d_row,
                              int d_col);

}  // namespace ss::core
