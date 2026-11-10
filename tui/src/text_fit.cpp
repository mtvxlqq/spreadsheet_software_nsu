#include "tui/text_fit.hpp"

#include <algorithm>
#include <vector>

#include <ftxui/screen/string.hpp>

namespace ss::tui {

namespace {

bool is_control(char c) {
    const auto byte = static_cast<unsigned char>(c);
    return byte < 0x20 || byte == 0x7F;
}

// Экранные символы строки. Utf8ToGlyphs выбрасывает управляющие символы, и
// многострочная ячейка из CSV слиплась бы, поэтому заранее меняем их на
// пробелы. Байт на байт, чтобы не сбить смещение каретки.
std::vector<std::string> glyphs_of(std::string_view text) {
    std::string printable(text);
    std::ranges::replace_if(printable, is_control, ' ');
    std::vector<std::string> glyphs = ftxui::Utf8ToGlyphs(printable);
    // после широкого символа FTXUI кладёт пустую заглушку, нам она не нужна
    std::erase_if(glyphs, [](const std::string& glyph) { return glyph.empty(); });
    return glyphs;
}

// номер символа, в котором лежит байт caret; в конце строки это glyphs.size()
std::size_t glyph_at(const std::vector<std::string>& glyphs, std::size_t caret) {
    std::size_t end = 0;
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        end += glyphs[i].size();
        if (caret < end) {
            return i;
        }
    }
    return glyphs.size();
}

std::string join(const std::vector<std::string>& glyphs, std::size_t first, std::size_t last) {
    std::string result;
    for (std::size_t i = first; i < last; ++i) {
        result += glyphs[i];
    }
    return result;
}

}  // namespace

std::string fit_cell(std::string_view text, int width, ss::app::Align align, Overflow overflow) {
    if (width <= 0) {
        return {};
    }
    std::string body;
    int used = 0;
    for (const std::string& glyph : glyphs_of(text)) {
        const int glyph_width = ftxui::string_width(glyph);
        if (used + glyph_width > width) {
            if (overflow == Overflow::Hashes) {
                return std::string(static_cast<std::size_t>(width), '#');
            }
            break;
        }
        body += glyph;
        used += glyph_width;
    }
    // при центрировании лишний пробел, если он есть, уходит вправо
    const int free = width - used;
    int left = 0;
    if (align == ss::app::Align::Right) {
        left = free;
    } else if (align == ss::app::Align::Center) {
        left = free / 2;
    }
    return std::string(static_cast<std::size_t>(left), ' ') + body +
           std::string(static_cast<std::size_t>(free - left), ' ');
}

CaretWindow caret_window(std::string_view text, std::size_t caret, int width) {
    if (width <= 0) {
        return {.before = "", .at = "", .after = ""};
    }
    const std::vector<std::string> glyphs = glyphs_of(text);
    const std::size_t at = glyph_at(glyphs, caret);
    const std::string at_text = at < glyphs.size() ? glyphs[at] : " ";
    int used = ftxui::string_width(at_text);
    // широкий символ в окне шириной 1
    if (used > width) {
        return {.before = "", .at = " ", .after = ""};
    }

    // сколько влезает слева от каретки, потом добираем справа
    std::size_t first = at;
    while (first > 0 && used + ftxui::string_width(glyphs[first - 1]) <= width) {
        --first;
        used += ftxui::string_width(glyphs[first]);
    }
    const std::size_t after_first = std::min(at + 1, glyphs.size());
    std::size_t last = after_first;
    while (last < glyphs.size() && used + ftxui::string_width(glyphs[last]) <= width) {
        used += ftxui::string_width(glyphs[last]);
        ++last;
    }
    return {.before = join(glyphs, first, at),
            .at = at_text,
            .after = join(glyphs, after_first, last)};
}

}  // namespace ss::tui
