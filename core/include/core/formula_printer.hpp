#pragma once

#include <string>

#include "core/expr_visitor.hpp"

namespace ss::core {

class Expr;

// Печатает дерево обратно в текст формулы. Скобки ставит только там, где без
// них формула разобралась бы по-другому: (1+2)*3, но 1+2*3.
class FormulaPrinter final : public ExprVisitor {
public:
    // текст начинается с '=', его можно снова отдать парсеру
    static std::string print(const Expr& expr);

    void visit(const NumberLit& node) override;
    void visit(const StringLit& node) override;
    void visit(const RefExpr& node) override;
    void visit(const RangeExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const FunctionCallExpr& node) override;

private:
    FormulaPrinter() = default;

    // Печатает операнд отдельным принтером и берёт его в скобки, если операнд
    // связывает слабее, чем min_precedence.
    static std::string print_operand(const Expr& operand, int min_precedence);

    std::string text_;
    // насколько сильно связывает напечатанное выражение, см. formula_printer.cpp
    int precedence_ = 0;
};

}  // namespace ss::core
