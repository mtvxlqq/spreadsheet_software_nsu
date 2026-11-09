#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ss::core {

// Свои проверки вместо std::isdigit и компании: те зависят от локали
// и падают на отрицательном char, то есть на любой русской букве.
constexpr bool is_ascii_digit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_ascii_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Регистр меняем только у латиницы. Имена функций и адреса и так ASCII.
std::string to_upper_ascii(std::string_view text);
std::string to_lower_ascii(std::string_view text);

// без пробелов, табов и переводов строк по краям
std::string_view trim_spaces(std::string_view text);

// Сколько байт в символе UTF-8, который начинается с этого байта.
// 0, если с такого байта символ начаться не может.
std::size_t utf8_char_length(unsigned char lead);

// байт вида 10xxxxxx, то есть середина символа
constexpr bool is_utf8_continuation(char c) {
    return (static_cast<unsigned char>(c) >> 6) == 0b10;
}

}  // namespace ss::core
