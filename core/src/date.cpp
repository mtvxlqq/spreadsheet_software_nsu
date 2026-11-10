#include "core/date.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <system_error>

#include "core/text.hpp"

namespace ss::core {

namespace {

// число из одних цифр длиной от min_length до max_length
std::optional<int> read_part(std::string_view text, std::size_t min_length,
                             std::size_t max_length) {
    if (text.size() < min_length || text.size() > max_length) {
        return std::nullopt;
    }
    for (const char c : text) {
        if (!is_ascii_digit(c)) {
            return std::nullopt;
        }
    }
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{}) {
        return std::nullopt;
    }
    return value;
}

// ровно три куска через separator, иначе nullopt
std::optional<std::array<std::string_view, 3>> split_in_three(std::string_view text,
                                                               char separator) {
    const std::size_t first = text.find(separator);
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t second = text.find(separator, first + 1);
    if (second == std::string_view::npos || text.find(separator, second + 1) != std::string_view::npos) {
        return std::nullopt;
    }
    return std::array{text.substr(0, first), text.substr(first + 1, second - first - 1),
                      text.substr(second + 1)};
}

const Date first_date = std::chrono::sys_days(std::chrono::year(min_year) / 1 / 1);
const Date last_date = std::chrono::sys_days(std::chrono::year(max_year) / 12 / 31);

}  // namespace

std::optional<Date> date_from_parts(int year, int month, int day) {
    if (year < min_year || year > max_year || month < 1 || month > 12 || day < 1 || day > 31) {
        return std::nullopt;
    }
    const std::chrono::year_month_day date{std::chrono::year(year),
                                           std::chrono::month(static_cast<unsigned>(month)),
                                           std::chrono::day(static_cast<unsigned>(day))};
    // ok() знает про длину месяцев и високосные годы
    if (!date.ok()) {
        return std::nullopt;
    }
    return std::chrono::sys_days(date);
}

std::optional<Date> parse_date(std::string_view text) {
    // год всегда из четырёх цифр, иначе 1.2.3 тоже стало бы датой
    if (const auto parts = split_in_three(text, '-')) {
        const std::optional<int> year = read_part((*parts)[0], 4, 4);
        const std::optional<int> month = read_part((*parts)[1], 1, 2);
        const std::optional<int> day = read_part((*parts)[2], 1, 2);
        if (year && month && day) {
            return date_from_parts(*year, *month, *day);
        }
    }
    if (const auto parts = split_in_three(text, '.')) {
        const std::optional<int> day = read_part((*parts)[0], 1, 2);
        const std::optional<int> month = read_part((*parts)[1], 1, 2);
        const std::optional<int> year = read_part((*parts)[2], 4, 4);
        if (year && month && day) {
            return date_from_parts(*year, *month, *day);
        }
    }
    return std::nullopt;
}

std::string format_date(Date date) {
    const std::chrono::year_month_day parts{date};
    return std::format("{:04}-{:02}-{:02}", static_cast<int>(parts.year()),
                       static_cast<unsigned>(parts.month()), static_cast<unsigned>(parts.day()));
}

std::optional<Date> shift_date(Date date, long long days) {
    // сравниваем числа дней, а не складываем сразу: сумма могла бы переполниться
    const long long from_start = (date - first_date).count();
    const long long total = (last_date - first_date).count();
    if (days < -from_start || days > total - from_start) {
        return std::nullopt;
    }
    return date + std::chrono::days(days);
}

}  // namespace ss::core
