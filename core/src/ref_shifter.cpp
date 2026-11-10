#include "core/ref_shifter.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/ref_mapping.hpp"

namespace ss::core {

ExprPtr RefShifter::shift(const Expr& expr, const RefMapping& mapping) {
    RefShifter shifter(mapping);
    return shifter.shift_child(expr);
}

ExprPtr RefShifter::shift_if_changed(const Expr& expr, const RefMapping& mapping) {
    RefShifter shifter(mapping);
    ExprPtr shifted = shifter.shift_child(expr);
    return shifter.changed_ ? std::move(shifted) : nullptr;
}

ExprPtr RefShifter::shift_child(const Expr& child) {
    child.accept(*this);
    return std::move(result_);
}

void RefShifter::visit(const NumberLit& node) {
    result_ = node.clone();
}

void RefShifter::visit(const StringLit& node) {
    result_ = node.clone();
}

void RefShifter::visit(const ErrorLit& node) {
    result_ = node.clone();
}

void RefShifter::visit(const RefExpr& node) {
    const std::optional<CellRef> moved = mapping_.map_ref(node.ref());
    changed_ = changed_ || moved != node.ref();
    if (moved) {
        result_ = std::make_unique<RefExpr>(*moved);
    } else {
        result_ = std::make_unique<ErrorLit>(ErrorCode::Ref);
    }
}

void RefShifter::visit(const RangeExpr& node) {
    const auto moved = mapping_.map_range(node.from(), node.to());
    changed_ = changed_ || moved != std::pair{node.from(), node.to()};
    if (moved) {
        result_ = std::make_unique<RangeExpr>(moved->first, moved->second);
    } else {
        result_ = std::make_unique<ErrorLit>(ErrorCode::Ref);
    }
}

void RefShifter::visit(const UnaryExpr& node) {
    result_ = std::make_unique<UnaryExpr>(node.op(), shift_child(node.operand()));
}

void RefShifter::visit(const BinaryExpr& node) {
    ExprPtr left = shift_child(node.left());
    ExprPtr right = shift_child(node.right());
    result_ = std::make_unique<BinaryExpr>(node.op(), std::move(left), std::move(right));
}

void RefShifter::visit(const FunctionCallExpr& node) {
    std::vector<ExprPtr> args;
    args.reserve(node.args().size());
    for (const ExprPtr& arg : node.args()) {
        args.push_back(shift_child(*arg));
    }
    result_ = node.with_args(std::move(args));
}

}  // namespace ss::core
