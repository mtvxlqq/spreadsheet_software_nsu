#include "core/formula_printer.hpp"

#include <format>
#include <string_view>

#include "core/expr.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

// Чем больше число, тем сильнее связывает операция. Уровни те же, что
// в грамматике парсера.
constexpr int comparison_precedence = 1;
constexpr int additive_precedence = 2;
constexpr int multiplicative_precedence = 3;
constexpr int unary_precedence = 4;
constexpr int primary_precedence = 5;

struct OpInfo {
    std::string_view text;
    int precedence;
};

OpInfo op_info(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:
            return {.text = "+", .precedence = additive_precedence};
        case BinaryOp::Sub:
            return {.text = "-", .precedence = additive_precedence};
        case BinaryOp::Mul:
            return {.text = "*", .precedence = multiplicative_precedence};
        case BinaryOp::Div:
            return {.text = "/", .precedence = multiplicative_precedence};
        case BinaryOp::Eq:
            return {.text = "=", .precedence = comparison_precedence};
        case BinaryOp::Ne:
            return {.text = "<>", .precedence = comparison_precedence};
        case BinaryOp::Lt:
            return {.text = "<", .precedence = comparison_precedence};
        case BinaryOp::Le:
            return {.text = "<=", .precedence = comparison_precedence};
        case BinaryOp::Gt:
            return {.text = ">", .precedence = comparison_precedence};
        case BinaryOp::Ge:
            return {.text = ">=", .precedence = comparison_precedence};
    }
    return {.text = "?", .precedence = comparison_precedence};  // сюда не попадём
}

// кавычка внутри строки удваивается, как при вводе
std::string quote(std::string_view text) {
    std::string result = "\"";
    for (const char c : text) {
        result += c;
        if (c == '"') {
            result += '"';
        }
    }
    return result + "\"";
}

}  // namespace

std::string FormulaPrinter::print(const Expr& expr) {
    return "=" + print_operand(expr, 0);
}

std::string FormulaPrinter::print_operand(const Expr& operand, int min_precedence) {
    FormulaPrinter printer;
    operand.accept(printer);
    if (printer.precedence_ < min_precedence) {
        return "(" + printer.text_ + ")";
    }
    return printer.text_;
}

void FormulaPrinter::visit(const NumberLit& node) {
    // {} печатает самую короткую запись, которая читается обратно в то же
    // число. Округлять до 10 знаков, как в ячейке, тут нельзя: изменится формула.
    text_ = std::format("{}", node.value());
    precedence_ = primary_precedence;
}

void FormulaPrinter::visit(const StringLit& node) {
    text_ = quote(node.text());
    precedence_ = primary_precedence;
}

void FormulaPrinter::visit(const RefExpr& node) {
    text_ = node.ref().to_string();
    precedence_ = primary_precedence;
}

void FormulaPrinter::visit(const RangeExpr& node) {
    text_ = node.from().to_string() + ":" + node.to().to_string();
    precedence_ = primary_precedence;
}

void FormulaPrinter::visit(const UnaryExpr& node) {
    const std::string sign = node.op() == UnaryOp::Neg ? "-" : "+";
    text_ = sign + print_operand(node.operand(), unary_precedence);
    precedence_ = unary_precedence;
}

void FormulaPrinter::visit(const BinaryExpr& node) {
    const OpInfo op = op_info(node.op());
    // Справа скобки нужны и при равном приоритете, иначе 1-(2-3) станет 1-2-3.
    // Слева при равном не нужны, кроме сравнений: цепочку 1=2=0 парсер не примет.
    const bool is_comparison = op.precedence == comparison_precedence;
    const int left_min = is_comparison ? op.precedence + 1 : op.precedence;
    text_ = print_operand(node.left(), left_min) + std::string(op.text) +
            print_operand(node.right(), op.precedence + 1);
    precedence_ = op.precedence;
}

void FormulaPrinter::visit(const FunctionCallExpr& node) {
    text_ = to_upper_ascii(node.name()) + "(";
    for (std::size_t i = 0; i < node.args().size(); ++i) {
        if (i > 0) {
            text_ += ",";
        }
        text_ += print_operand(*node.args()[i], 0);
    }
    text_ += ")";
    precedence_ = primary_precedence;
}

}  // namespace ss::core
