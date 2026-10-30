#include "core/address.hpp"

#include <algorithm>

#include "core/errors.hpp"

namespace ss::core {

namespace {

bool isAsciiLetter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool isAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

// Буквы столбца → индекс с нуля. Слишком длинное имя «упирается» в kMaxCols —
// первый столбец за пределами листа. Так не бывает переполнения int.
int columnFromLetters(std::string_view letters) {
    int number = 0;  // нумерация с единицы: A = 1, Z = 26, AA = 27
    for (const char c : letters) {
        const int letter = (c >= 'a') ? c - 'a' : c - 'A';
        number = (number * 26) + letter + 1;
        if (number > kMaxCols) {
            return kMaxCols;
        }
    }
    return number - 1;
}

// Номер строки → индекс с нуля. Слишком большой номер «упирается» в kMaxRows.
int rowFromDigits(std::string_view digits) {
    int number = 0;
    for (const char c : digits) {
        number = (number * 10) + (c - '0');
        if (number > kMaxRows) {
            return kMaxRows;
        }
    }
    return number - 1;  // "A0" даст -1: тоже за пределами листа
}

}  // namespace

std::string columnName(int col) {
    std::string name;
    for (int number = col + 1; number > 0; number = (number - 1) / 26) {
        name.insert(name.begin(), static_cast<char>('A' + ((number - 1) % 26)));
    }
    return name;
}

CellAddress CellAddress::parse(std::string_view text) {
    const std::optional<CellRef> ref = CellRef::tryParse(text);
    if (!ref || ref->absRow || ref->absCol || !ref->addr.isValid()) {
        throw AddressError("неверный адрес ячейки: \"" + std::string(text) + "\"");
    }
    return ref->addr;
}

bool CellAddress::isValid() const {
    return row >= 0 && row < kMaxRows && col >= 0 && col < kMaxCols;
}

std::string CellAddress::toString() const {
    return columnName(col) + std::to_string(row + 1);
}

std::optional<CellRef> CellRef::tryParse(std::string_view text) {
    CellRef ref;
    std::size_t pos = 0;
    if (pos < text.size() && text[pos] == '$') {
        ref.absCol = true;
        ++pos;
    }
    const std::size_t lettersStart = pos;
    while (pos < text.size() && isAsciiLetter(text[pos])) {
        ++pos;
    }
    const std::string_view letters = text.substr(lettersStart, pos - lettersStart);
    if (pos < text.size() && text[pos] == '$') {
        ref.absRow = true;
        ++pos;
    }
    const std::size_t digitsStart = pos;
    while (pos < text.size() && isAsciiDigit(text[pos])) {
        ++pos;
    }
    const std::string_view digits = text.substr(digitsStart, pos - digitsStart);

    if (letters.empty() || digits.empty() || pos != text.size()) {
        return std::nullopt;
    }
    ref.addr = CellAddress{.row = rowFromDigits(digits), .col = columnFromLetters(letters)};
    return ref;
}

std::string CellRef::toString() const {
    std::string text;
    if (absCol) {
        text += '$';
    }
    text += columnName(addr.col);
    if (absRow) {
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

void Range::forEach(const std::function<void(CellAddress)>& fn) const {
    for (int row = from_.row; row <= to_.row; ++row) {
        for (int col = from_.col; col <= to_.col; ++col) {
            fn(CellAddress{.row = row, .col = col});
        }
    }
}

std::string Range::toString() const {
    return from_.toString() + ":" + to_.toString();
}

}  // namespace ss::core
