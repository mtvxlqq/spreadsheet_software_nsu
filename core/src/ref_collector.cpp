#include "core/ref_collector.hpp"

#include <string>

#include "core/errors.hpp"
#include "core/expr.hpp"

namespace ss::core {

std::set<CellAddress> RefCollector::collect(const Expr& expr) {
    RefCollector collector;
    expr.accept(collector);
    return std::move(collector.refs_);
}

void RefCollector::visit(const NumberLit& /*node*/) {}

void RefCollector::visit(const StringLit& /*node*/) {}

void RefCollector::visit(const RefExpr& node) {
    // ссылка за пределы листа всё равно даст #REF!, следить за ней незачем
    if (node.ref().addr.is_valid()) {
        refs_.insert(node.ref().addr);
    }
}

void RefCollector::visit(const RangeExpr& node) {
    const CellAddress from = node.from().addr;
    const CellAddress to = node.to().addr;
    if (!from.is_valid() || !to.is_valid()) {
        return;
    }
    const Range range(from, to);
    // проверяем до разворачивания, чтобы не выделять память под миллионы ячеек
    if (refs_.size() + range.size() > max_formula_refs) {
        throw LimitError("в формуле слишком большие диапазоны (больше " +
                         std::to_string(max_formula_refs) + " ячеек)");
    }
    range.for_each([this](CellAddress address) { refs_.insert(address); });
}

void RefCollector::visit(const UnaryExpr& node) {
    node.operand().accept(*this);
}

void RefCollector::visit(const BinaryExpr& node) {
    node.left().accept(*this);
    node.right().accept(*this);
}

void RefCollector::visit(const FunctionCallExpr& node) {
    for (const ExprPtr& arg : node.args()) {
        arg->accept(*this);
    }
}

}  // namespace ss::core
