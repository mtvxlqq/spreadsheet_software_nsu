#include "core/io/csv_reader.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/address.hpp"
#include "core/cell_io.hpp"
#include "core/errors.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

// Разбирает поток по одной записи. Номер строки считаем сами: запись
// с переводом строки в кавычках занимает несколько строк файла.
class CsvParser {
public:
    explicit CsvParser(std::istream& in) : in_(in) {
        skip_bom();
    }

    // nullopt, когда файл кончился
    std::optional<std::vector<std::string>> next_record() {
        if (in_.peek() == eof) {
            return std::nullopt;
        }
        record_line_ = line_;
        std::vector<std::string> fields;
        bool record_ended = false;
        while (!record_ended) {
            std::string field;
            record_ended = read_field(field);
            fields.push_back(std::move(field));
        }
        return fields;
    }

    // строка файла, с которой началась последняя запись
    std::size_t record_line() const {
        return record_line_;
    }

private:
    static constexpr int eof = std::char_traits<char>::eof();

    // Excel любит писать UTF-8 с BOM в начале
    void skip_bom() {
        if (in_.peek() != 0xEF) {
            return;
        }
        in_.get();
        if (in_.get() != 0xBB || in_.get() != 0xBF) {
            throw FileFormatError("непонятные байты в начале файла", 1);
        }
    }

    // съедает \n или \r\n, если они дальше
    bool line_break(int c) {
        if (c == '\r' && in_.peek() == '\n') {
            in_.get();
            c = '\n';
        }
        if (c == '\n') {
            ++line_;
            return true;
        }
        return false;
    }

    // true, если на этом поле запись кончилась
    bool read_field(std::string& field) {
        if (in_.peek() == '"') {
            in_.get();
            read_quoted(field);
            const int c = in_.get();
            if (c == eof || line_break(c)) {
                return true;
            }
            if (c != ',') {
                throw FileFormatError("после закрывающей кавычки должна быть запятая", line_);
            }
            return false;
        }
        while (true) {
            const int c = in_.get();
            if (c == eof || line_break(c)) {
                return true;
            }
            if (c == ',') {
                return false;
            }
            // кавычку посреди поля без кавычек берём как обычный символ
            field += static_cast<char>(c);
        }
    }

    void read_quoted(std::string& field) {
        const std::size_t start = line_;
        while (true) {
            const int c = in_.get();
            if (c == eof) {
                throw FileFormatError("кавычка не закрыта", start);
            }
            if (c == '"') {
                if (in_.peek() != '"') {
                    return;
                }
                in_.get();
            }
            if (line_break(c)) {
                field += '\n';
            } else {
                field += static_cast<char>(c);
            }
        }
    }

    std::istream& in_;
    std::size_t line_ = 1;
    std::size_t record_line_ = 1;
};

// Чтобы "=1+1" не стал формулой, а "'x" не потерял апостроф, ставим
// впереди ещё один апостроф. CellInputParser его снимет.
std::string as_literal(const std::string& field) {
    const std::string_view text = trim_spaces(field);
    if (!text.empty() && (text.front() == '=' || text.front() == '\'')) {
        return "'" + std::string(text);
    }
    return field;
}

}  // namespace

std::string_view CsvReader::name() const {
    return "CSV";
}

std::vector<std::string> CsvReader::extensions() const {
    return {".csv"};
}

void CsvReader::load(std::istream& in, CellSink& sink) const {
    CsvParser parser(in);
    int row = 0;
    while (const std::optional<std::vector<std::string>> fields = parser.next_record()) {
        for (std::size_t col = 0; col < fields->size(); ++col) {
            const std::string& field = (*fields)[col];
            if (trim_spaces(field).empty()) {
                continue;
            }
            const CellAddress address{.row = row, .col = static_cast<int>(col)};
            if (col >= static_cast<std::size_t>(max_cols) || !address.is_valid()) {
                throw FileFormatError("таблица не помещается в лист", parser.record_line());
            }
            sink.set_input(address, as_literal(field));
        }
        ++row;
    }
    if (in.bad()) {
        throw FileFormatError("не удалось дочитать файл");
    }
}

}  // namespace ss::core
