#pragma once

namespace ss::core {

class NumberLit;
class StringLit;
class RefExpr;
class RangeExpr;
class UnaryExpr;
class BinaryExpr;
class FunctionCallExpr;

// Операция над деревом формулы (паттерн Visitor). Новая операция - это новый
// посетитель, а классы узлов при этом не меняются.
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    ExprVisitor(const ExprVisitor&) = delete;
    ExprVisitor& operator=(const ExprVisitor&) = delete;
    ExprVisitor(ExprVisitor&&) = delete;
    ExprVisitor& operator=(ExprVisitor&&) = delete;

    virtual void visit(const NumberLit& node) = 0;
    virtual void visit(const StringLit& node) = 0;
    virtual void visit(const RefExpr& node) = 0;
    virtual void visit(const RangeExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const FunctionCallExpr& node) = 0;

protected:
    ExprVisitor() = default;
};

}  // namespace ss::core
