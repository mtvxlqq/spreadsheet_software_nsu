#include "core/recalculator.hpp"

#include "core/cell.hpp"
#include "core/dependency_graph.hpp"
#include "core/eval_context.hpp"
#include "core/evaluator.hpp"
#include "core/sheet.hpp"

namespace ss::core {

std::vector<CellAddress> Recalculator::run(const RecalcPlan& plan, Sheet& sheet,
                                           EvalContext& context) {
    std::vector<CellAddress> touched;
    touched.reserve(plan.order.size() + plan.cyclic.size());

    for (const CellAddress address : plan.cyclic) {
        const Cell* cell = sheet.get(address);
        if (cell != nullptr && cell->formula) {
            sheet.set_value(address, make_error(ErrorCode::Cycle));
        }
    }
    // В order попадают и ячейки без формул (например, только что введённое
    // число) и даже пустые. Их значение уже известно, считать нечего.
    for (const CellAddress address : plan.order) {
        const Cell* cell = sheet.get(address);
        if (cell != nullptr && cell->formula) {
            sheet.set_value(address, Evaluator::evaluate_formula(*cell->formula, context));
        }
        touched.push_back(address);
    }
    touched.insert(touched.end(), plan.cyclic.begin(), plan.cyclic.end());
    return touched;
}

}  // namespace ss::core
