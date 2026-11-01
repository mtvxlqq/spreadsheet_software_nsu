#include "core/expr.hpp"

#include "core/expr_visitor.hpp"

namespace ss::core {

void NumberLit::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr NumberLit::clone() const {
    return std::make_unique<NumberLit>(value_);
}

void StringLit::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr StringLit::clone() const {
    return std::make_unique<StringLit>(text_);
}

void RefExpr::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr RefExpr::clone() const {
    return std::make_unique<RefExpr>(ref_);
}

void RangeExpr::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr RangeExpr::clone() const {
    return std::make_unique<RangeExpr>(from_, to_);
}

void UnaryExpr::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr UnaryExpr::clone() const {
    return std::make_unique<UnaryExpr>(op_, operand_->clone());
}

void BinaryExpr::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr BinaryExpr::clone() const {
    return std::make_unique<BinaryExpr>(op_, left_->clone(), right_->clone());
}

void FunctionCallExpr::accept(ExprVisitor& visitor) const {
    visitor.visit(*this);
}

ExprPtr FunctionCallExpr::clone() const {
    std::vector<ExprPtr> args;
    args.reserve(args_.size());
    for (const ExprPtr& arg : args_) {
        args.push_back(arg->clone());
    }
    return std::make_unique<FunctionCallExpr>(name_, std::move(args), function_);
}

}  // namespace ss::core
