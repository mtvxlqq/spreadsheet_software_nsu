#include "core/evaluator.hpp"

#include <compare>
#include <optional>
#include <utility>
#include <vector>

#include "core/expr.hpp"
#include "core/function.hpp"

namespace ss::core {

namespace {

bool is_comparison(BinaryOp op) {
    return op == BinaryOp::Eq || op == BinaryOp::Ne || op == BinaryOp::Lt ||
           op == BinaryOp::Le || op == BinaryOp::Gt || op == BinaryOp::Ge;
}

ValuePtr arithmetic(BinaryOp op, double left, double right) {
    switch (op) {
        case BinaryOp::Add:
            return make_number_or_error(left + right);
        case BinaryOp::Sub:
            return make_number_or_error(left - right);
        case BinaryOp::Mul:
            return make_number_or_error(left * right);
        case BinaryOp::Div:
            if (right == 0) {
                return make_error(ErrorCode::Div0);
            }
            return make_number_or_error(left / right);
        default:
            return make_error(ErrorCode::Value);  // сравнения сюда не попадают
    }
}

bool holds(BinaryOp op, std::partial_ordering order) {
    switch (op) {
        case BinaryOp::Eq:
            return order == 0;
        case BinaryOp::Ne:
            return order != 0;
        case BinaryOp::Lt:
            return order < 0;
        case BinaryOp::Le:
            return order <= 0;
        case BinaryOp::Gt:
            return order > 0;
        case BinaryOp::Ge:
            return order >= 0;
        default:
            return false;
    }
}

// Логического типа на этом этапе нет, поэтому истина это 1, а ложь 0.
// Строки сравниваются без учёта регистра, число со строкой сравнить нельзя.
ValuePtr comparison(BinaryOp op, const Value& left, const Value& right) {
    const std::optional<std::partial_ordering> order = left.compare(right);
    if (!order) {
        return make_error(ErrorCode::Value);
    }
    return make_number(holds(op, *order) ? 1 : 0);
}

}  // namespace

ValuePtr Evaluator::evaluate(const Expr& expr, EvalContext& context) {
    Evaluator evaluator(context, false);
    expr.accept(evaluator);
    return evaluator.result_.values.front();
}

ArgValues Evaluator::evaluate_flat(const Expr& expr, EvalContext& context) {
    Evaluator evaluator(context, true);
    expr.accept(evaluator);
    return std::move(evaluator.result_);
}

ValuePtr Evaluator::evaluate_formula(const Expr& expr, EvalContext& context) {
    ValuePtr value = evaluate(expr, context);
    if (value->is_empty()) {
        return make_number(0);
    }
    return value;
}

void Evaluator::set_result(ValuePtr value) {
    result_.values.assign(1, std::move(value));
    result_.from_range = false;
}

void Evaluator::visit(const NumberLit& node) {
    set_result(make_number(node.value()));
}

void Evaluator::visit(const StringLit& node) {
    set_result(make_string(node.text()));
}

void Evaluator::visit(const RefExpr& node) {
    const CellAddress address = node.ref().addr;
    if (!address.is_valid()) {
        set_result(make_error(ErrorCode::Ref));
        return;
    }
    set_result(context_.value_at(address));
}

void Evaluator::visit(const RangeExpr& node) {
    const CellAddress from = node.from().addr;
    const CellAddress to = node.to().addr;
    if (!from.is_valid() || !to.is_valid()) {
        set_result(make_error(ErrorCode::Ref));
        return;
    }
    // диапазон можно передать только в функцию, =A1:B2+1 это ошибка
    if (!flatten_) {
        set_result(make_error(ErrorCode::Value));
        return;
    }
    const Range range(from, to);
    result_.values.clear();
    result_.values.reserve(range.size());
    range.for_each([this](CellAddress address) {
        result_.values.push_back(context_.value_at(address));
    });
    result_.from_range = true;
}

void Evaluator::visit(const UnaryExpr& node) {
    const ValuePtr operand = evaluate(node.operand(), context_);
    if (operand->is_error()) {
        set_result(operand);
        return;
    }
    const std::optional<double> number = operand->as_number();
    if (!number) {
        set_result(make_error(ErrorCode::Value));
        return;
    }
    set_result(make_number(node.op() == UnaryOp::Neg ? -*number : *number));
}

// Ошибки ищем слева направо: в =1/0+A99999999 будет #DIV/0!, а не #REF!
void Evaluator::visit(const BinaryExpr& node) {
    const ValuePtr left = evaluate(node.left(), context_);
    const ValuePtr right = evaluate(node.right(), context_);
    if (left->is_error()) {
        set_result(left);
        return;
    }
    if (right->is_error()) {
        set_result(right);
        return;
    }
    if (is_comparison(node.op())) {
        set_result(comparison(node.op(), *left, *right));
        return;
    }
    const std::optional<double> left_number = left->as_number();
    const std::optional<double> right_number = right->as_number();
    if (!left_number || !right_number) {
        set_result(make_error(ErrorCode::Value));
        return;
    }
    set_result(arithmetic(node.op(), *left_number, *right_number));
}

void Evaluator::visit(const FunctionCallExpr& node) {
    const Function* function = node.function();
    if (function == nullptr) {
        set_result(make_error(ErrorCode::Name));
        return;
    }
    // Функции получают аргументы деревьями и сами решают, что вычислять.
    std::vector<const Expr*> args;
    args.reserve(node.args().size());
    for (const ExprPtr& arg : node.args()) {
        args.push_back(arg.get());
    }
    set_result(function->call(args, context_));
}

}  // namespace ss::core
