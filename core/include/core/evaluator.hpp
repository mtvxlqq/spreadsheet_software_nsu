#pragma once

#include "core/eval_context.hpp"
#include "core/expr_visitor.hpp"
#include "core/value.hpp"

namespace ss::core {

class Expr;

// Считает дерево формулы. Ошибки (#DIV/0!, #REF! и т.д.) возвращаются
// значениями, доменных исключений тут нет. Значения других ячеек берутся
// готовыми из контекста, поэтому рекурсии между ячейками не бывает.
class Evaluator final : public ExprVisitor {
public:
    static ValuePtr evaluate(const Expr& expr, EvalContext& context);
    static ArgValues evaluate_flat(const Expr& expr, EvalContext& context);

    // Итог формулы для ячейки: то же, что evaluate, но пустое значение
    // превращается в 0 (=A1 при пустой A1 показывает 0).
    static ValuePtr evaluate_formula(const Expr& expr, EvalContext& context);

    void visit(const NumberLit& node) override;
    void visit(const StringLit& node) override;
    void visit(const RefExpr& node) override;
    void visit(const RangeExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const FunctionCallExpr& node) override;

private:
    Evaluator(EvalContext& context, bool flatten) : context_(context), flatten_(flatten) {}

    void set_result(ValuePtr value);

    EvalContext& context_;
    // разворачивать ли диапазон, если он окажется корнем дерева
    bool flatten_;
    ArgValues result_;
};

}  // namespace ss::core
