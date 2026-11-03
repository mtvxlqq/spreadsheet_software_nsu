#pragma once

#include "core/eval_context.hpp"

namespace ss::core {

class Sheet;

// EvalContext поверх листа. Вынесен из Spreadsheet, чтобы evaluate()
// не торчал в публичном интерфейсе фасада.
class SheetEvalContext final : public EvalContext {
public:
    explicit SheetEvalContext(const Sheet& sheet) : sheet_(sheet) {}

    ValuePtr value_at(CellAddress address) const override;
    ValuePtr evaluate(const Expr& expr) override;
    ArgValues evaluate_flat(const Expr& expr) override;

private:
    const Sheet& sheet_;
};

}  // namespace ss::core
