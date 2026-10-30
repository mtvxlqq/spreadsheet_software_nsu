#pragma once

#include <cstddef>
#include <string_view>

#include "core/token.hpp"

namespace ss::core {

// Разбивает формулу на токены. Приоритетами операций и деревом
// занимается уже Parser.
class Lexer {
public:
    // хранит string_view, так что строка должна жить дольше лексера
    explicit Lexer(std::string_view text);

    // В конце формулы возвращает End, сколько ни вызывай.
    // На лишние символы, незакрытую кавычку и т.п. бросает ParseError.
    Token next();

private:
    // за концом строки возвращает '\0', чтобы не проверять границы везде
    char peek(std::size_t offset = 0) const;
    void skip_spaces();
    void skip_digits();
    Token read_number();
    Token read_string();
    Token read_word();
    Token read_operator();

    std::string_view text_;
    std::size_t pos_ = 0;
};

}  // namespace ss::core
