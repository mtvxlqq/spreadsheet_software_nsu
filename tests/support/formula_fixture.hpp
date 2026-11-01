#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/builtins.hpp"
#include "core/evaluator.hpp"
#include "core/function_registry.hpp"
#include "core/parser.hpp"
#include "core/value.hpp"
#include "support/fake_eval_context.hpp"

namespace ss::test {

// Реестр со встроенными функциями и лист из словаря: формулу можно
// разобрать и посчитать одной строкой.
class FormulaTest : public ::testing::Test {
protected:
    FormulaTest() {
        core::register_builtins(functions);
    }

    core::ValuePtr evaluate(std::string_view formula) {
        return core::Evaluator::evaluate_formula(*core::Parser::parse(formula, functions), sheet);
    }

    // как результат покажется в ячейке: "3", "abc", "#DIV/0!"
    std::string eval(std::string_view formula) {
        return evaluate(formula)->to_text();
    }

    // чтобы отличить число 1 от строки "1"
    std::optional<double> number(std::string_view formula) {
        return evaluate(formula)->as_number();
    }

    core::FunctionRegistry functions;
    FakeEvalContext sheet;
};

}  // namespace ss::test
