#include "core/parser.hpp"

#include <charconv>
#include <memory>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#include "core/address.hpp"
#include "core/errors.hpp"
#include "core/function.hpp"

namespace ss::core {

namespace {

std::optional<BinaryOp> comparison_op(TokenType type) {
    switch (type) {
        case TokenType::Equal:
            return BinaryOp::Eq;
        case TokenType::NotEqual:
            return BinaryOp::Ne;
        case TokenType::Less:
            return BinaryOp::Lt;
        case TokenType::LessEqual:
            return BinaryOp::Le;
        case TokenType::Greater:
            return BinaryOp::Gt;
        case TokenType::GreaterEqual:
            return BinaryOp::Ge;
        default:
            return std::nullopt;
    }
}

// что стоит на месте ошибки, для сообщения
std::string describe(const Token& token) {
    switch (token.type) {
        case TokenType::End:
            return "конец формулы";
        case TokenType::String:
            return "строка";
        default:
            return "'" + token.text + "'";
    }
}

}  // namespace

ExprPtr Parser::parse(std::string_view formula, const FunctionProvider& functions) {
    if (formula.size() > max_length) {
        throw ParseError("формула длиннее " + std::to_string(max_length) + " символов",
                         max_length);
    }
    Parser parser(formula, functions);
    return parser.parse_formula();
}

Parser::Parser(std::string_view formula, const FunctionProvider& functions)
    : lexer_(formula), functions_(functions) {
    current_ = lexer_.next();
}

ExprPtr Parser::parse_formula() {
    expect(TokenType::Equal, "'=' в начале формулы");
    ExprPtr result = parse_comparison();
    if (current_.type != TokenType::End) {
        fail("конец формулы");
    }
    return result;
}

// Сравнение не цепляется в цепочку: в =1<2<3 легко ошибиться,
// так что без скобок это синтаксическая ошибка.
ExprPtr Parser::parse_comparison() {
    ExprPtr left = parse_additive();
    const std::optional<BinaryOp> op = comparison_op(current_.type);
    if (!op) {
        return left;
    }
    advance();
    ExprPtr right = parse_additive();
    return std::make_unique<BinaryExpr>(*op, std::move(left), std::move(right));
}

// цикл, а не рекурсия: так 1-2-3 собирается как (1-2)-3
ExprPtr Parser::parse_additive() {
    ExprPtr left = parse_multiplicative();
    while (current_.type == TokenType::Plus || current_.type == TokenType::Minus) {
        const BinaryOp op = advance().type == TokenType::Plus ? BinaryOp::Add : BinaryOp::Sub;
        ExprPtr right = parse_multiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parse_multiplicative() {
    ExprPtr left = parse_unary();
    while (current_.type == TokenType::Star || current_.type == TokenType::Slash) {
        const BinaryOp op = advance().type == TokenType::Star ? BinaryOp::Mul : BinaryOp::Div;
        ExprPtr right = parse_unary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// Через unary проходит любая вложенность (скобки и аргументы функций тоже),
// поэтому глубину считаем здесь. Уменьшать depth_ после исключения не нужно:
// парсер после ошибки уже не используется.
ExprPtr Parser::parse_unary() {
    if (++depth_ > max_depth) {
        throw ParseError("слишком глубокая вложенность", current_.pos);
    }
    ExprPtr result;
    if (current_.type == TokenType::Minus || current_.type == TokenType::Plus) {
        const UnaryOp op = advance().type == TokenType::Minus ? UnaryOp::Neg : UnaryOp::Plus;
        result = std::make_unique<UnaryExpr>(op, parse_unary());
    } else {
        result = parse_primary();
    }
    --depth_;
    return result;
}

ExprPtr Parser::parse_primary() {
    switch (current_.type) {
        case TokenType::Number:
            return parse_number();
        case TokenType::String:
            return std::make_unique<StringLit>(advance().text);
        case TokenType::Ref:
            return parse_reference();
        case TokenType::Ident:
            return parse_call();
        case TokenType::LParen: {
            advance();
            ExprPtr inner = parse_comparison();
            expect(TokenType::RParen, "')'");
            return inner;
        }
        default:
            fail("число, строка, ссылка, функция или '('");
    }
}

// лексер уже проверил запись числа, остаётся только перевести в double
ExprPtr Parser::parse_number() {
    const std::string& text = current_.text;
    double value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error == std::errc::result_out_of_range) {
        throw ParseError("число " + text + " вне допустимого диапазона", current_.pos);
    }
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw ParseError("не получилось прочитать число " + text, current_.pos);
    }
    advance();
    return std::make_unique<NumberLit>(value);
}

ExprPtr Parser::parse_reference() {
    // лексер выдаёт Ref, только если try_parse его понял, так что value() не бросит
    const CellRef from = CellRef::try_parse(advance().text).value();
    if (current_.type != TokenType::Colon) {
        return std::make_unique<RefExpr>(from);
    }
    advance();
    const CellRef to = CellRef::try_parse(expect(TokenType::Ref, "ссылка после ':'").text).value();
    return std::make_unique<RangeExpr>(from, to);
}

ExprPtr Parser::parse_call() {
    const Token name = advance();
    expect(TokenType::LParen, "'(' после имени " + name.text);
    std::vector<ExprPtr> args;
    if (current_.type != TokenType::RParen) {
        args.push_back(parse_comparison());
        while (current_.type == TokenType::Comma) {
            advance();
            args.push_back(parse_comparison());
        }
    }
    expect(TokenType::RParen, "',' или ')'");
    return std::make_unique<FunctionCallExpr>(name.text, std::move(args),
                                              functions_.create(name.text));
}

Token Parser::advance() {
    Token token = std::move(current_);
    current_ = lexer_.next();
    return token;
}

Token Parser::expect(TokenType type, const std::string& what) {
    if (current_.type != type) {
        fail(what);
    }
    return advance();
}

void Parser::fail(const std::string& expected) const {
    throw ParseError("ожидалось " + expected + ", а встретилось " + describe(current_),
                     current_.pos);
}

}  // namespace ss::core
