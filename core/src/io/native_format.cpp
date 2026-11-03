#include "core/io/native_format.hpp"

#include <cstddef>
#include <string>

#include "core/address.hpp"
#include "core/cell_io.hpp"
#include "core/errors.hpp"

namespace ss::core {

namespace {

constexpr std::string_view signature = "SST 1";

std::string escape(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char c : text) {
        if (c == '\\') {
            result += "\\\\";
        } else if (c == '\t') {
            result += "\\t";
        } else if (c == '\n') {
            result += "\\n";
        } else {
            result += c;
        }
    }
    return result;
}

std::string unescape(std::string_view text, std::size_t line) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\\') {
            result += text[i];
            continue;
        }
        if (i + 1 == text.size()) {
            throw FileFormatError("строка кончается на \\", line);
        }
        const char next = text[++i];
        if (next == '\\') {
            result += '\\';
        } else if (next == 't') {
            result += '\t';
        } else if (next == 'n') {
            result += '\n';
        } else {
            throw FileFormatError(std::string("неизвестная последовательность \\") + next, line);
        }
    }
    return result;
}

// файл могли сохранить в Windows. В конце raw \r быть не может,
// CellInputParser обрезает пробелы по краям.
void drop_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

void check_signature(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        throw FileFormatError("файл пустой, а должен начинаться с SST 1", 1);
    }
    drop_cr(line);
    if (line == signature) {
        return;
    }
    if (line.starts_with("SST ")) {
        throw FileFormatError("версия " + line.substr(4) + " не поддерживается", 1);
    }
    throw FileFormatError("это не файл .sst, нет строки SST 1", 1);
}

}  // namespace

std::string_view NativeFormat::name() const {
    return "SST";
}

std::vector<std::string> NativeFormat::extensions() const {
    return {".sst"};
}

void NativeFormat::load(std::istream& in, CellSink& sink) const {
    check_signature(in);
    std::string line;
    std::size_t number = 1;
    while (std::getline(in, line)) {
        ++number;
        drop_cr(line);
        if (line.empty()) {
            continue;
        }
        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            throw FileFormatError("нет таба между адресом и содержимым", number);
        }
        // ParseError и AddressError переделываем, чтобы был номер строки
        try {
            const CellAddress address = CellAddress::parse(std::string_view(line).substr(0, tab));
            sink.set_input(address, unescape(std::string_view(line).substr(tab + 1), number));
        } catch (const FileFormatError&) {
            throw;
        } catch (const Error& error) {
            throw FileFormatError(error.what(), number);
        }
    }
    if (in.bad()) {
        throw FileFormatError("не удалось дочитать файл");
    }
}

void NativeFormat::save(std::ostream& out, const CellSource& source) const {
    out << signature << '\n';
    source.for_each_cell([&](CellAddress address) {
        out << address.to_string() << '\t' << escape(source.raw(address)) << '\n';
    });
    out.flush();
    if (!out) {
        throw FileFormatError("не удалось записать файл");
    }
}

}  // namespace ss::core
