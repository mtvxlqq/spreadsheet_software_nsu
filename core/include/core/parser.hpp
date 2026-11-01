#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "core/expr.hpp"
#include "core/lexer.hpp"
#include "core/token.hpp"

namespace ss::core {

class FunctionProvider;

// Разбор формулы рекурсивным спуском. Каждому уровню грамматики своя функция,
// чем ниже уровень, тем сильнее связывает операция:
//
//   formula        := '=' comparison End
//   comparison     := additive ( ('=' | '<>' | '<' | '<=' | '>' | '>=') additive )?
//   additive       := multiplicative ( ('+' | '-') multiplicative )*
//   multiplicative := unary ( ('*' | '/') unary )*
//   unary          := ('-' | '+') unary | primary
//   primary        := Number | String | Ref ( ':' Ref )?
//                   | Ident '(' ( comparison ( ',' comparison )* )? ')'
//                   | '(' comparison ')'
class Parser {
public:
    // Текст начинается с '='. При синтаксической ошибке бросает ParseError
    // с позицией. Неизвестная функция ошибкой не считается: в узле будет
    // nullptr, и при вычислении получится #NAME?
    static ExprPtr parse(std::string_view formula, const FunctionProvider& functions);

    // длиннее не принимаем, чтобы огромная формула не съела стек (в Excel так же)
    static constexpr std::size_t max_length = 8192;
    // сколько раз можно вложить скобки, вызовы функций и унарные минусы
    static constexpr int max_depth = 100;

private:
    Parser(std::string_view formula, const FunctionProvider& functions);

    ExprPtr parse_formula();
    ExprPtr parse_comparison();
    ExprPtr parse_additive();
    ExprPtr parse_multiplicative();
    ExprPtr parse_unary();
    ExprPtr parse_primary();
    ExprPtr parse_number();
    ExprPtr parse_reference();
    ExprPtr parse_call();

    // текущий токен отдаём, а следующий читаем из лексера
    Token advance();
    // как advance(), но если токен не того типа, бросает ParseError
    Token expect(TokenType type, const std::string& what);
    [[noreturn]] void fail(const std::string& expected) const;

    Lexer lexer_;
    const FunctionProvider& functions_;
    Token current_;
    int depth_ = 0;
};

}  // namespace ss::core
