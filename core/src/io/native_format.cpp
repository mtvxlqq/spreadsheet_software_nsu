#include "core/io/native_format.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "core/address.hpp"
#include "core/cell_io.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/errors.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

constexpr std::string_view signature = "SST 2";
// в первой версии не было оформления, такие файлы читаем как раньше
constexpr std::string_view old_signature = "SST 1";
constexpr std::string_view style_mark = "!style";
constexpr std::string_view width_mark = "!width";
// так в файле записан обычный формат чисел, в стиле это пустая строка
constexpr std::string_view general_name = "general";

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

// Файл могли сохранить в Windows, там строки кончаются на \r\n. Своего \r
// в конце raw быть не может: CellInputParser всё равно обрезает края.
void drop_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

// номер версии из первой строки файла
int read_version(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        throw FileFormatError("файл пустой, а должен начинаться с SST 2", 1);
    }
    drop_cr(line);
    if (line == signature) {
        return 2;
    }
    if (line == old_signature) {
        return 1;
    }
    if (line.starts_with("SST ")) {
        throw FileFormatError("версия " + line.substr(4) + " не поддерживается", 1);
    }
    throw FileFormatError("это не файл .sst, нет строки SST 2", 1);
}

std::vector<std::string_view> split_by_tab(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t tab = line.find('\t', start);
        if (tab == std::string_view::npos) {
            fields.push_back(line.substr(start));
            return fields;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
}

std::string_view align_name(const std::optional<Align>& align) {
    if (!align) {
        return "auto";
    }
    switch (*align) {
        case Align::Left:
            return "left";
        case Align::Right:
            return "right";
        case Align::Center:
            return "center";
    }
    return "auto";  // сюда не попадём, но без return gcc ругается
}

std::optional<Align> parse_align(std::string_view name) {
    if (name == "auto") {
        return std::nullopt;
    }
    if (name == "left") {
        return Align::Left;
    }
    if (name == "right") {
        return Align::Right;
    }
    if (name == "center") {
        return Align::Center;
    }
    throw Error("неизвестное выравнивание " + std::string(name));
}

// "B" это 1; цифры в имени столбца не допускаются
int parse_column(std::string_view name) {
    if (name.empty() || !std::ranges::all_of(name, is_ascii_letter)) {
        throw AddressError("кривое имя столбца " + std::string(name));
    }
    return CellAddress::parse(std::string(name) + "1").col;
}

int parse_width(std::string_view text) {
    int width = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), width);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw Error("ширина столбца должна быть числом, а не " + std::string(text));
    }
    return width;
}

void read_cell(std::string_view line, std::size_t number, CellSink& sink) {
    const std::size_t tab = line.find('\t');
    if (tab == std::string_view::npos) {
        throw FileFormatError("нет таба между адресом и содержимым", number);
    }
    const CellAddress address = CellAddress::parse(line.substr(0, tab));
    sink.set_input(address, unescape(line.substr(tab + 1), number));
}

// строка оформления: "!style адрес формат выравнивание" или "!width столбец ширина"
void read_directive(std::string_view line, std::size_t number, CellSink& sink) {
    const std::vector<std::string_view> fields = split_by_tab(line);
    if (fields[0] == style_mark) {
        if (fields.size() != 4) {
            throw FileFormatError("в строке !style должны быть адрес, формат и выравнивание",
                                  number);
        }
        // формат не проверяем: неизвестный покажется как обычное число,
        // а при сохранении запишется обратно как был
        const std::string format = fields[2] == general_name ? "" : unescape(fields[2], number);
        sink.set_style(CellAddress::parse(fields[1]),
                       {.format = format, .align = parse_align(fields[3])});
    } else if (fields[0] == width_mark) {
        if (fields.size() != 3) {
            throw FileFormatError("в строке !width должны быть столбец и ширина", number);
        }
        sink.set_column_width(parse_column(fields[1]), parse_width(fields[2]));
    } else {
        throw FileFormatError("неизвестная служебная строка " + std::string(fields[0]), number);
    }
}

}  // namespace

std::string_view NativeFormat::name() const {
    return "SST";
}

std::vector<std::string> NativeFormat::extensions() const {
    return {".sst"};
}

void NativeFormat::load(std::istream& in, CellSink& sink) const {
    const int version = read_version(in);
    std::string line;
    std::size_t number = 1;
    while (std::getline(in, line)) {
        ++number;
        drop_cr(line);
        if (line.empty()) {
            continue;
        }
        // ParseError, AddressError и LimitError переделываем, чтобы был номер строки
        try {
            if (line.front() != '!') {
                read_cell(line, number, sink);
            } else if (version >= 2) {
                read_directive(line, number, sink);
            } else {
                throw FileFormatError("в файле SST 1 не бывает строк оформления", number);
            }
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
    source.for_each_style([&](CellAddress address, const CellStyle& style) {
        const std::string format = style.format.empty() ? std::string(general_name)
                                                        : escape(style.format);
        out << style_mark << '\t' << address.to_string() << '\t' << format << '\t'
            << align_name(style.align) << '\n';
    });
    for (const auto& [col, width] : source.column_widths().custom()) {
        out << width_mark << '\t' << column_name(col) << '\t' << width << '\n';
    }
    out.flush();
    if (!out) {
        throw FileFormatError("не удалось записать файл");
    }
}

}  // namespace ss::core
