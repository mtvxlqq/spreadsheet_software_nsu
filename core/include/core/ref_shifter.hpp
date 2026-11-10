#pragma once

#include "core/expr.hpp"
#include "core/expr_visitor.hpp"

namespace ss::core {

class RefMapping;

// Строит копию дерева, в которой ссылки сдвинуты по правилу mapping.
// Новое дерево, а не правка старого: узлы остаются неизменяемыми, и если
// где-то случится исключение, исходная формула не пострадает.
class RefShifter final : public ExprVisitor {
public:
    static ExprPtr shift(const Expr& expr, const RefMapping& mapping);

    // То же, но nullptr, если ни одна ссылка не сдвинулась. Тогда формулу
    // можно оставить как есть и не печатать заново.
    static ExprPtr shift_if_changed(const Expr& expr, const RefMapping& mapping);

    void visit(const NumberLit& node) override;
    void visit(const StringLit& node) override;
    void visit(const ErrorLit& node) override;
    void visit(const RefExpr& node) override;
    void visit(const RangeExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const FunctionCallExpr& node) override;

private:
    explicit RefShifter(const RefMapping& mapping) : mapping_(mapping) {}

    // обходит поддерево этим же посетителем, чтобы changed_ был общим
    ExprPtr shift_child(const Expr& child);

    const RefMapping& mapping_;
    ExprPtr result_;
    bool changed_ = false;
};

}  // namespace ss::core
