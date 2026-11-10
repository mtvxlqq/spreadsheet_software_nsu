#include "core/ref_shifter.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/ref_mapping.hpp"

namespace ss::core {

ExprPtr RefShifter::shift(const Expr& expr, const RefMapping& mapping) {
    RefShifter shifter(mapping);
    expr.accept(shifter);
    return std::move(shifter.result_);
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
    if (moved) {
        result_ = std::make_unique<RefExpr>(*moved);
    } else {
        result_ = std::make_unique<ErrorLit>(ErrorCode::Ref);
    }
}

void RefShifter::visit(const RangeExpr& node) {
    const auto moved = mapping_.map_range(node.from(), node.to());
    if (moved) {
        result_ = std::make_unique<RangeExpr>(moved->first, moved->second);
    } else {
        result_ = std::make_unique<ErrorLit>(ErrorCode::Ref);
    }
}

void RefShifter::visit(const UnaryExpr& node) {
    result_ = std::make_unique<UnaryExpr>(node.op(), shift(node.operand(), mapping_));
}

void RefShifter::visit(const BinaryExpr& node) {
    ExprPtr left = shift(node.left(), mapping_);
    ExprPtr right = shift(node.right(), mapping_);
    result_ = std::make_unique<BinaryExpr>(node.op(), std::move(left), std::move(right));
}

void RefShifter::visit(const FunctionCallExpr& node) {
    std::vector<ExprPtr> args;
    args.reserve(node.args().size());
    for (const ExprPtr& arg : node.args()) {
        args.push_back(shift(*arg, mapping_));
    }
    result_ = node.with_args(std::move(args));
}

}  // namespace ss::core
