#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell.hpp"
#include "core/cell_input.hpp"
#include "core/dependency_graph.hpp"
#include "core/eval_context.hpp"
#include "core/evaluator.hpp"
#include "core/function_registry.hpp"
#include "core/recalculator.hpp"
#include "core/ref_collector.hpp"
#include "core/sheet.hpp"
#include "core/value.hpp"

using ss::core::CellAddress;

namespace {

// Склеиваем разбор ввода, граф, лист и пересчёт так же, как это потом
// сделает Spreadsheet, только без строгой гарантии и наблюдателей.
class Recalc : public ::testing::Test, public ss::core::EvalContext {
protected:
    Recalc() {
        ss::core::register_builtins(functions);
    }

    // возвращает адреса, которые пересчитались, в виде "A1 B1 C1"
    std::string input(std::string_view address_text, std::string_view raw) {
        const CellAddress address = CellAddress::parse(address_text);
        std::optional<ss::core::Cell> cell = parser.parse(raw);
        if (cell && cell->formula) {
            graph.set_dependencies(address, ss::core::RefCollector::collect(*cell->formula));
        } else {
            graph.remove_cell(address);
        }
        if (cell) {
            sheet.set(address, std::move(*cell));
        } else {
            sheet.remove(address);
        }
        const std::vector<CellAddress> touched =
            ss::core::Recalculator::run(graph.plan({address}), sheet, *this);
        std::string result;
        for (const CellAddress touched_address : touched) {
            result += (result.empty() ? "" : " ") + touched_address.to_string();
        }
        return result;
    }

    std::string shown(std::string_view address_text) const {
        return value_at(CellAddress::parse(address_text))->to_text();
    }

    ss::core::ValuePtr value_at(CellAddress address) const override {
        const ss::core::Cell* cell = sheet.get(address);
        return cell == nullptr ? ss::core::empty_value() : cell->value;
    }

    ss::core::ValuePtr evaluate(const ss::core::Expr& expr) override {
        return ss::core::Evaluator::evaluate(expr, *this);
    }

    ss::core::ArgValues evaluate_flat(const ss::core::Expr& expr) override {
        return ss::core::Evaluator::evaluate_flat(expr, *this);
    }

    ss::core::FunctionRegistry functions;
    ss::core::CellInputParser parser{functions};
    ss::core::Sheet sheet;
    ss::core::DependencyGraph graph;
};

}  // namespace

TEST_F(Recalc, ChainFollowsChange) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("C1", "=B1*2");
    EXPECT_EQ(shown("C1"), "4");

    EXPECT_EQ(input("A1", "10"), "A1 B1 C1");
    EXPECT_EQ(shown("B1"), "11");
    EXPECT_EQ(shown("C1"), "22");
}

TEST_F(Recalc, OnlyAffectedCellsAreTouched) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("D1", "=5*5");
    EXPECT_EQ(input("A1", "2"), "A1 B1");
    EXPECT_EQ(shown("D1"), "25");
}

TEST_F(Recalc, FormulaWithoutReferences) {
    EXPECT_EQ(input("A1", "=2*3"), "A1");
    EXPECT_EQ(shown("A1"), "6");
}

TEST_F(Recalc, EmptyCellInsideRangeUpdatesSum) {
    input("A1", "1");
    input("B1", "=SUM(A1:A3)");
    EXPECT_EQ(shown("B1"), "1");
    EXPECT_EQ(input("A2", "5"), "A2 B1");
    EXPECT_EQ(shown("B1"), "6");
}

TEST_F(Recalc, CycleGivesCycleError) {
    input("A1", "=B1");
    EXPECT_EQ(shown("A1"), "0");
    input("B1", "=A1");
    EXPECT_EQ(shown("A1"), "#CYCLE!");
    EXPECT_EQ(shown("B1"), "#CYCLE!");

    input("C1", "=A1+1");
    EXPECT_EQ(shown("C1"), "#CYCLE!");

    EXPECT_EQ(input("B1", "5"), "B1 A1 C1");
    EXPECT_EQ(shown("A1"), "5");
    EXPECT_EQ(shown("C1"), "6");
}

TEST_F(Recalc, SelfReference) {
    input("A1", "=A1+1");
    EXPECT_EQ(shown("A1"), "#CYCLE!");
}

TEST_F(Recalc, ClearingCellUpdatesDependents) {
    input("A1", "7");
    input("B1", "=A1*2");
    EXPECT_EQ(input("A1", ""), "A1 B1");
    EXPECT_EQ(shown("B1"), "0");
}
