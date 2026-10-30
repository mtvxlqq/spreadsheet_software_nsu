#pragma once

#include <cstddef>
#include <string>

namespace ss::core {

enum class TokenType {
    Number,  // 12, 3.5, .5, 1e-3
    String,
    Ref,    // A1, $B$2
    Ident,  // имя функции
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    LParen,
    RParen,
    Comma,
    Colon,
    End,
};

struct Token {
    TokenType type = TokenType::End;
    // у String тут уже готовая строка: без кавычек, "" заменено на "
    std::string text;
    // где токен начинается в формуле (в байтах), нужно для сообщений об ошибках
    std::size_t pos = 0;

    bool operator==(const Token&) const = default;
};

}  // namespace ss::core
