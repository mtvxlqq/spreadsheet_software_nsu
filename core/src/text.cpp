#include "core/text.hpp"

namespace ss::core {

std::string to_upper_ascii(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return result;
}

std::string to_lower_ascii(std::string_view text) {
    std::string result(text);
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

std::string_view trim_spaces(std::string_view text) {
    while (!text.empty() && is_space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && is_space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

std::size_t utf8_char_length(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead >> 5) == 0b110) {
        return 2;
    }
    if ((lead >> 4) == 0b1110) {
        return 3;
    }
    if ((lead >> 3) == 0b11110) {
        return 4;
    }
    return 0;
}

}  // namespace ss::core
