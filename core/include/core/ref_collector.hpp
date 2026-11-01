#pragma once

#include <cstddef>
#include <set>

#include "core/address.hpp"
#include "core/expr_visitor.hpp"

namespace ss::core {

class Expr;

// Сколько ячеек может собрать одна формула. Три полных столбца влезают,
// а SUM(A1:ZZ65536) на 46 млн ячеек уже нет: граф зависимостей съел бы
// гигабайты памяти.
inline constexpr std::size_t max_formula_refs = 200'000;

// Собирает ячейки, от которых зависит формула. Диапазон разворачивается
// в отдельные ячейки, включая пустые: иначе ввод в пустую ячейку внутри
// SUM(A1:A10) не пересчитал бы сумму.
class RefCollector final : public ExprVisitor {
public:
    // если ячеек больше max_formula_refs, бросает LimitError
    static std::set<CellAddress> collect(const Expr& expr);

    void visit(const NumberLit& node) override;
    void visit(const StringLit& node) override;
    void visit(const RefExpr& node) override;
    void visit(const RangeExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const FunctionCallExpr& node) override;

private:
    RefCollector() = default;

    std::set<CellAddress> refs_;
};

}  // namespace ss::core
