#include "core/address.hpp"

#include <algorithm>

#include "core/errors.hpp"

namespace ss::core {

namespace {

bool is_ascii_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_ascii_digit(char c) {
    return c >= '0' && c <= '9';
}

// "AB" -> 27. Если букв слишком много, сразу возвращаем max_cols (первый
// столбец за листом), иначе на длинных именах переполнится int.
int column_from_letters(std::string_view letters) {
    int number = 0;  // A=1, Z=26, AA=27
    for (const char c : letters) {
        const int letter = (c >= 'a') ? c - 'a' : c - 'A';
        number = (number * 26) + letter + 1;
        if (number > max_cols) {
            return max_cols;
        }
    }
    return number - 1;
}

// "12" -> 11. Слишком большой номер обрезаем до max_rows по той же причине.
int row_from_digits(std::string_view digits) {
    int number = 0;
    for (const char c : digits) {
        number = (number * 10) + (c - '0');
        if (number > max_rows) {
            return max_rows;
        }
    }
    return number - 1;  // у "A0" выйдет -1, это тоже вне листа
}

}  // namespace

std::string column_name(int col) {
    std::string name;
    for (int number = col + 1; number > 0; number = (number - 1) / 26) {
        name.insert(name.begin(), static_cast<char>('A' + ((number - 1) % 26)));
    }
    return name;
}

CellAddress CellAddress::parse(std::string_view text) {
    const std::optional<CellRef> ref = CellRef::try_parse(text);
    if (!ref || ref->abs_row || ref->abs_col || !ref->addr.is_valid()) {
        throw AddressError("неверный адрес ячейки: \"" + std::string(text) + "\"");
    }
    return ref->addr;
}

bool CellAddress::is_valid() const {
    return row >= 0 && row < max_rows && col >= 0 && col < max_cols;
}

std::string CellAddress::to_string() const {
    return column_name(col) + std::to_string(row + 1);
}

std::optional<CellRef> CellRef::try_parse(std::string_view text) {
    CellRef ref;
    std::size_t pos = 0;
    if (pos < text.size() && text[pos] == '$') {
        ref.abs_col = true;
        ++pos;
    }
    const std::size_t letters_start = pos;
    while (pos < text.size() && is_ascii_letter(text[pos])) {
        ++pos;
    }
    const std::string_view letters = text.substr(letters_start, pos - letters_start);
    if (pos < text.size() && text[pos] == '$') {
        ref.abs_row = true;
        ++pos;
    }
    const std::size_t digits_start = pos;
    while (pos < text.size() && is_ascii_digit(text[pos])) {
        ++pos;
    }
    const std::string_view digits = text.substr(digits_start, pos - digits_start);

    if (letters.empty() || digits.empty() || pos != text.size()) {
        return std::nullopt;
    }
    ref.addr = CellAddress{.row = row_from_digits(digits), .col = column_from_letters(letters)};
    return ref;
}

std::string CellRef::to_string() const {
    std::string text;
    if (abs_col) {
        text += '$';
    }
    text += column_name(addr.col);
    if (abs_row) {
        text += '$';
    }
    text += std::to_string(addr.row + 1);
    return text;
}

Range::Range(CellAddress corner1, CellAddress corner2)
    : from_{.row = std::min(corner1.row, corner2.row), .col = std::min(corner1.col, corner2.col)},
      to_{.row = std::max(corner1.row, corner2.row), .col = std::max(corner1.col, corner2.col)} {}

bool Range::contains(CellAddress address) const {
    return address.row >= from_.row && address.row <= to_.row && address.col >= from_.col &&
           address.col <= to_.col;
}

std::size_t Range::size() const {
    const int rows = to_.row - from_.row + 1;
    const int cols = to_.col - from_.col + 1;
    return static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
}

void Range::for_each(const std::function<void(CellAddress)>& fn) const {
    for (int row = from_.row; row <= to_.row; ++row) {
        for (int col = from_.col; col <= to_.col; ++col) {
            fn(CellAddress{.row = row, .col = col});
        }
    }
}

std::string Range::to_string() const {
    return from_.to_string() + ":" + to_.to_string();
}

}  // namespace ss::core
