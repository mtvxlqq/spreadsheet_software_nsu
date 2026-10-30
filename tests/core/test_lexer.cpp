#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/errors.hpp"
#include "core/lexer.hpp"
#include "support/printers.hpp"

using ss::core::Lexer;
using ss::core::ParseError;
using ss::core::Token;
using ss::core::TokenType;

namespace {

// все токены, кроме End
std::vector<Token> tokenize(std::string_view text) {
    Lexer lexer(text);
    std::vector<Token> tokens;
    for (Token token = lexer.next(); token.type != TokenType::End; token = lexer.next()) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<TokenType> types_of(std::string_view text) {
    std::vector<TokenType> types;
    for (const Token& token : tokenize(text)) {
        types.push_back(token.type);
    }
    return types;
}

// для формул из одного токена
Token single(std::string_view text) {
    const std::vector<Token> tokens = tokenize(text);
    EXPECT_EQ(tokens.size(), 1U) << text;
    return tokens.empty() ? Token{} : tokens.front();
}

void expect_error_at(std::string_view text, std::size_t position) {
    try {
        tokenize(text);
        ADD_FAILURE() << "нет ParseError для: " << text;
    } catch (const ParseError& error) {
        EXPECT_EQ(error.position(), position) << text;
    }
}

}  // namespace

TEST(Lexer, ReadsNumbers) {
    for (const std::string text : {"1", "1.5", ".5", "1.", "1e3", "1E-3", "2.5e+10", "007"}) {
        EXPECT_EQ(single(text), (Token{.type = TokenType::Number, .text = text, .pos = 0}));
    }
}

TEST(Lexer, RejectsExponentWithoutDigits) {
    expect_error_at("1e", 0);
    expect_error_at("=2+1E+", 3);
}

TEST(Lexer, ReadsStringsWithDoubledQuotes) {
    EXPECT_EQ(single(R"("abc")").text, "abc");
    EXPECT_EQ(single(R"("a""b")").text, R"(a"b)");
    EXPECT_EQ(single(R"("""")").text, R"(")");
    EXPECT_EQ(single(R"("")").text, "");
    EXPECT_EQ(single(R"("with  spaces")").text, "with  spaces");
    EXPECT_EQ(single(R"("Привет")").text, "Привет");
    EXPECT_EQ(single(R"("abc")").type, TokenType::String);
}

TEST(Lexer, ReportsUnterminatedStringAtOpeningQuote) {
    expect_error_at(R"("abc)", 0);
    expect_error_at(R"(=1+"abc)", 3);
    expect_error_at(R"(="a"")", 1);  // "" тут кавычка внутри строки, закрывающей нет
}

TEST(Lexer, ReadsReferencesAsWritten) {
    for (const std::string text : {"A1", "$A$1", "A$1", "$A1", "b2", "ZZ65536"}) {
        EXPECT_EQ(single(text), (Token{.type = TokenType::Ref, .text = text, .pos = 0}));
    }
}

TEST(Lexer, KeepsReferencesOutsideSheet) {
    EXPECT_EQ(single("A99999999").type, TokenType::Ref);
    EXPECT_EQ(single("AAA1").type, TokenType::Ref);
}

TEST(Lexer, ReadsIdentifiers) {
    EXPECT_EQ(single("SUM"), (Token{.type = TokenType::Ident, .text = "SUM", .pos = 0}));
    EXPECT_EQ(single("sum").type, TokenType::Ident);
    EXPECT_EQ(single("my_func").type, TokenType::Ident);
}

TEST(Lexer, WordBeforeParenthesisIsFunctionName) {
    EXPECT_EQ(types_of("LOG10(1)"), (std::vector{TokenType::Ident, TokenType::LParen,
                                                 TokenType::Number, TokenType::RParen}));
    EXPECT_EQ(tokenize("A1(").front().type, TokenType::Ident);
    // а с пробелом это уже ссылка
    EXPECT_EQ(tokenize("A1 (").front().type, TokenType::Ref);
}

TEST(Lexer, ReadsAllOperators) {
    EXPECT_EQ(
        types_of("+ - * / = <> < <= > >= ( ) , :"),
        (std::vector{TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash,
                     TokenType::Equal, TokenType::NotEqual, TokenType::Less, TokenType::LessEqual,
                     TokenType::Greater, TokenType::GreaterEqual, TokenType::LParen,
                     TokenType::RParen, TokenType::Comma, TokenType::Colon}));
}

TEST(Lexer, TwoCharOperatorsWithoutSpaces) {
    EXPECT_EQ(types_of("1<=2<>3>=4"),
              (std::vector{TokenType::Number, TokenType::LessEqual, TokenType::Number,
                           TokenType::NotEqual, TokenType::Number, TokenType::GreaterEqual,
                           TokenType::Number}));
}

TEST(Lexer, SkipsSpacesAndKeepsPositions) {
    const std::vector<Token> tokens = tokenize("= A1 +\t 2");
    ASSERT_EQ(tokens.size(), 4U);
    EXPECT_EQ(tokens[0].pos, 0U);
    EXPECT_EQ(tokens[1].pos, 2U);
    EXPECT_EQ(tokens[2].pos, 5U);
    EXPECT_EQ(tokens[3].pos, 8U);
}

TEST(Lexer, ReturnsEndRepeatedly) {
    Lexer lexer("=1 ");
    lexer.next();
    lexer.next();
    EXPECT_EQ(lexer.next(), (Token{.type = TokenType::End, .text = "", .pos = 3}));
    EXPECT_EQ(lexer.next().type, TokenType::End);
}

TEST(Lexer, EmptyTextIsJustEnd) {
    EXPECT_TRUE(tokenize("").empty());
    EXPECT_TRUE(tokenize("   ").empty());
}

TEST(Lexer, TokenizesWholeFormula) {
    EXPECT_EQ(types_of("=SUM(A1:B2, 3) * -C1"),
              (std::vector{TokenType::Equal, TokenType::Ident, TokenType::LParen, TokenType::Ref,
                           TokenType::Colon, TokenType::Ref, TokenType::Comma, TokenType::Number,
                           TokenType::RParen, TokenType::Star, TokenType::Minus, TokenType::Ref}));
}

TEST(Lexer, RejectsUnknownCharacters) {
    expect_error_at("=1#2", 2);
    expect_error_at("=A1!", 3);
    expect_error_at("=.", 1);
    expect_error_at("=2^3", 2);
}

TEST(Lexer, ExplainsCyrillicLetterOutsideQuotes) {
    // тут русская А
    try {
        tokenize("=А1");
        FAIL() << "нет ParseError";
    } catch (const ParseError& error) {
        EXPECT_EQ(error.position(), 1U);
        EXPECT_NE(std::string(error.what()).find("'А'"), std::string::npos) << error.what();
    }
}
