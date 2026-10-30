#include "core/lexer.hpp"

#include <array>
#include <string>

#include "core/address.hpp"
#include "core/errors.hpp"

namespace ss::core {

namespace {

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// '$' считаем частью слова из-за ссылок вида $A$1
bool is_word_start(char c) {
    return is_letter(c) || c == '_' || c == '$';
}

bool is_word_char(char c) {
    return is_word_start(c) || is_digit(c);
}

struct OperatorSpelling {
    std::string_view text;
    TokenType type;
};

// двухсимвольные стоят первыми, иначе "<=" разберётся как "<" и "="
constexpr std::array<OperatorSpelling, 14> operator_spellings{{
    {.text = "<=", .type = TokenType::LessEqual},
    {.text = "<>", .type = TokenType::NotEqual},
    {.text = ">=", .type = TokenType::GreaterEqual},
    {.text = "+", .type = TokenType::Plus},
    {.text = "-", .type = TokenType::Minus},
    {.text = "*", .type = TokenType::Star},
    {.text = "/", .type = TokenType::Slash},
    {.text = "=", .type = TokenType::Equal},
    {.text = "<", .type = TokenType::Less},
    {.text = ">", .type = TokenType::Greater},
    {.text = "(", .type = TokenType::LParen},
    {.text = ")", .type = TokenType::RParen},
    {.text = ",", .type = TokenType::Comma},
    {.text = ":", .type = TokenType::Colon},
}};

// берём символ UTF-8 целиком, чтобы в сообщении не было половины русской буквы
std::string utf8_char_at(std::string_view text, std::size_t pos) {
    const auto lead = static_cast<unsigned char>(text[pos]);
    std::size_t length = 1;
    if (lead >= 0xF0) {
        length = 4;
    } else if (lead >= 0xE0) {
        length = 3;
    } else if (lead >= 0xC0) {
        length = 2;
    }
    return std::string(text.substr(pos, length));
}

}  // namespace

Lexer::Lexer(std::string_view text) : text_(text) {}

Token Lexer::next() {
    skip_spaces();
    const char c = peek();
    if (pos_ >= text_.size()) {
        return Token{.type = TokenType::End, .text = "", .pos = pos_};
    }
    if (is_digit(c) || (c == '.' && is_digit(peek(1)))) {
        return read_number();
    }
    if (c == '"') {
        return read_string();
    }
    if (is_word_start(c)) {
        return read_word();
    }
    return read_operator();
}

char Lexer::peek(std::size_t offset) const {
    const std::size_t index = pos_ + offset;
    return index < text_.size() ? text_[index] : '\0';
}

void Lexer::skip_spaces() {
    while (pos_ < text_.size() && is_space(text_[pos_])) {
        ++pos_;
    }
}

void Lexer::skip_digits() {
    while (is_digit(peek())) {
        ++pos_;
    }
}

// 12, 1.5, .5, 1e-3. Тут только проверяем запись, в double переводит парсер.
Token Lexer::read_number() {
    const std::size_t start = pos_;
    skip_digits();
    if (peek() == '.') {
        ++pos_;
        skip_digits();
    }
    if (peek() == 'e' || peek() == 'E') {
        ++pos_;
        if (peek() == '+' || peek() == '-') {
            ++pos_;
        }
        if (!is_digit(peek())) {
            throw ParseError("после 'e' в числе должны идти цифры", start);
        }
        skip_digits();
    }
    return Token{.type = TokenType::Number,
                 .text = std::string(text_.substr(start, pos_ - start)),
                 .pos = start};
}

// кавычка внутри строки пишется двойной: "a""b" это a"b
Token Lexer::read_string() {
    const std::size_t start = pos_;
    ++pos_;  // пропускаем открывающую кавычку
    std::string value;
    while (pos_ < text_.size()) {
        const char c = text_[pos_++];
        if (c != '"') {
            value += c;
        } else if (peek() == '"') {
            value += '"';
            ++pos_;
        } else {
            return Token{.type = TokenType::String, .text = value, .pos = start};
        }
    }
    throw ParseError("строка не закрыта кавычкой", start);
}

Token Lexer::read_word() {
    const std::size_t start = pos_;
    while (is_word_char(peek())) {
        ++pos_;
    }
    std::string word(text_.substr(start, pos_ - start));
    // LOG10 похоже на адрес, поэтому если сразу за словом идёт скобка, это функция
    const bool is_ref = peek() != '(' && CellRef::try_parse(word).has_value();
    return Token{
        .type = is_ref ? TokenType::Ref : TokenType::Ident, .text = std::move(word), .pos = start};
}

Token Lexer::read_operator() {
    const std::string_view rest = text_.substr(pos_);
    for (const OperatorSpelling& op : operator_spellings) {
        if (rest.starts_with(op.text)) {
            const std::size_t start = pos_;
            pos_ += op.text.size();
            return Token{.type = op.type, .text = std::string(op.text), .pos = start};
        }
    }

    const std::string symbol = utf8_char_at(text_, pos_);
    if (static_cast<unsigned char>(symbol[0]) >= 0x80) {
        // чаще всего это русская А в адресе вместо латинской
        throw ParseError(
            "недопустимый символ '" + symbol + "': вне кавычек в формуле допустима только латиница",
            pos_);
    }
    throw ParseError("неожиданный символ '" + symbol + "'", pos_);
}

}  // namespace ss::core
