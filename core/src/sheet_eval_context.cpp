#include "sheet_eval_context.hpp"

#include "core/cell.hpp"
#include "core/evaluator.hpp"
#include "core/sheet.hpp"

namespace ss::core {

ValuePtr SheetEvalContext::value_at(CellAddress address) const {
    const Cell* cell = sheet_.get(address);
    return cell == nullptr ? empty_value() : cell->value;
}

ValuePtr SheetEvalContext::evaluate(const Expr& expr) {
    return Evaluator::evaluate(expr, *this);
}

ArgValues SheetEvalContext::evaluate_flat(const Expr& expr) {
    return Evaluator::evaluate_flat(expr, *this);
}

}  // namespace ss::core
