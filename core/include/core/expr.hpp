#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/address.hpp"

namespace ss::core {

class ExprVisitor;
class Function;

enum class UnaryOp { Neg, Plus };
enum class BinaryOp { Add, Sub, Mul, Div, Eq, Ne, Lt, Le, Gt, Ge };

class Expr;
using ExprPtr = std::unique_ptr<Expr>;

// Узел дерева формулы (паттерн Composite). После разбора дерево не меняется,
// поэтому accept() константный: посетители только читают узлы.
class Expr {
public:
    virtual ~Expr() = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
    Expr(Expr&&) = delete;
    Expr& operator=(Expr&&) = delete;

    virtual void accept(ExprVisitor& visitor) const = 0;
    // глубокая копия, понадобится на втором этапе при копировании ячеек
    virtual ExprPtr clone() const = 0;

protected:
    Expr() = default;
};

class NumberLit final : public Expr {
public:
    explicit NumberLit(double value) : value_(value) {}

    double value() const {
        return value_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    double value_;
};

class StringLit final : public Expr {
public:
    explicit StringLit(std::string text) : text_(std::move(text)) {}

    const std::string& text() const {
        return text_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    std::string text_;
};

class RefExpr final : public Expr {
public:
    explicit RefExpr(CellRef ref) : ref_(ref) {}

    const CellRef& ref() const {
        return ref_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    CellRef ref_;
};

// A1:B3 в формуле. Углы храним как их написали, в Range они превращаются
// только при вычислении.
class RangeExpr final : public Expr {
public:
    RangeExpr(CellRef from, CellRef to) : from_(from), to_(to) {}

    const CellRef& from() const {
        return from_;
    }

    const CellRef& to() const {
        return to_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    CellRef from_;
    CellRef to_;
};

class UnaryExpr final : public Expr {
public:
    UnaryExpr(UnaryOp op, ExprPtr operand) : op_(op), operand_(std::move(operand)) {}

    UnaryOp op() const {
        return op_;
    }

    const Expr& operand() const {
        return *operand_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    UnaryOp op_;
    ExprPtr operand_;
};

class BinaryExpr final : public Expr {
public:
    BinaryExpr(BinaryOp op, ExprPtr left, ExprPtr right)
        : op_(op), left_(std::move(left)), right_(std::move(right)) {}

    BinaryOp op() const {
        return op_;
    }

    const Expr& left() const {
        return *left_;
    }

    const Expr& right() const {
        return *right_;
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    BinaryOp op_;
    ExprPtr left_;
    ExprPtr right_;
};

// Вызов функции. Функцию ищем по имени ещё при разборе; если такой нет,
// function() вернёт nullptr, и при вычислении получится #NAME?
class FunctionCallExpr final : public Expr {
public:
    FunctionCallExpr(std::string name, std::vector<ExprPtr> args,
                     std::shared_ptr<const Function> function)
        : name_(std::move(name)), args_(std::move(args)), function_(std::move(function)) {}

    // как написал пользователь, например "sum"
    const std::string& name() const {
        return name_;
    }

    const std::vector<ExprPtr>& args() const {
        return args_;
    }

    const Function* function() const {
        return function_.get();
    }

    void accept(ExprVisitor& visitor) const override;
    ExprPtr clone() const override;

private:
    std::string name_;
    std::vector<ExprPtr> args_;
    // shared_ptr, потому что у функции нет состояния: clone() просто делит её
    // с копией, и ходить за новой в реестр не нужно
    std::shared_ptr<const Function> function_;
};

}  // namespace ss::core
