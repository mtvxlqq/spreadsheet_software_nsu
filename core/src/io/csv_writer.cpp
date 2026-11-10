#include "core/io/csv_writer.hpp"

#include <optional>
#include <string>

#include "core/address.hpp"
#include "core/cell_io.hpp"
#include "core/errors.hpp"

namespace ss::core {

namespace {

// Поле с запятой, кавычкой или переводом строки берётся в кавычки,
// а кавычка внутри удваивается.
std::string csv_field(const std::string& text) {
    if (text.find_first_of(",\"\r\n") == std::string::npos) {
        return text;
    }
    std::string quoted = "\"";
    for (const char c : text) {
        if (c == '"') {
            quoted += '"';
        }
        quoted += c;
    }
    quoted += '"';
    return quoted;
}

}  // namespace

std::string_view CsvWriter::name() const {
    return "CSV";
}

std::vector<std::string> CsvWriter::extensions() const {
    return {".csv"};
}

void CsvWriter::save(std::ostream& out, const CellSource& source) const {
    const std::optional<Range> used = source.used_range();
    if (used) {
        // от A1, а не от первой заполненной ячейки: тогда при повторном
        // импорте всё окажется на своих местах
        for (int row = 0; row <= used->to().row; ++row) {
            for (int col = 0; col <= used->to().col; ++col) {
                if (col > 0) {
                    out << ',';
                }
                out << csv_field(source.value({.row = row, .col = col})->to_text());
            }
            // по RFC 4180 строки кончаются на \r\n
            out << "\r\n";
        }
    }
    out.flush();
    if (!out) {
        throw FileFormatError("не удалось записать файл");
    }
}

}  // namespace ss::core
