#include "core/formula_rewrite.hpp"

#include "core/expr.hpp"
#include "core/formula_printer.hpp"
#include "core/parser.hpp"
#include "core/ref_mapping.hpp"
#include "core/ref_shifter.hpp"
#include "core/text.hpp"

namespace ss::core {

std::string rewrite_formula(std::string_view raw, const FunctionProvider& functions,
                            const RefMapping& mapping) {
    const std::string_view text = trim_spaces(raw);
    if (!text.starts_with('=')) {
        return std::string(raw);
    }
    const ExprPtr formula = Parser::parse(text, functions);
    const ExprPtr shifted = RefShifter::shift_if_changed(*formula, mapping);
    if (!shifted) {
        return std::string(raw);
    }
    return FormulaPrinter::print(*shifted);
}

std::string translate_formula(std::string_view raw, const FunctionProvider& functions, int d_row,
                              int d_col) {
    return rewrite_formula(raw, functions, CopyMapping(d_row, d_col));
}

}  // namespace ss::core
