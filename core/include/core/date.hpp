#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace ss::core {

// Календарная дата без времени. Внутри это число дней от 1970-01-01,
// поэтому разность двух дат считается обычным вычитанием.
using Date = std::chrono::sys_days;

// годы, с которыми работаем; за их пределами в формуле получится #VALUE!
inline constexpr int min_year = 1;
inline constexpr int max_year = 9999;

// nullopt, если такой даты нет (31 февраля) или год вне min_year..max_year
std::optional<Date> date_from_parts(int year, int month, int day);

// "2024-03-15" или "15.03.2024". nullopt, если это не дата или даты
// с такими числами не бывает.
std::optional<Date> parse_date(std::string_view text);

// всегда в виде 2024-03-15
std::string format_date(Date date);

// дата через days дней; nullopt, если вышли за min_year..max_year
std::optional<Date> shift_date(Date date, long long days);

}  // namespace ss::core
