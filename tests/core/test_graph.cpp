#include <algorithm>
#include <initializer_list>
#include <set>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/dependency_graph.hpp"
#include "support/printers.hpp"

using ss::core::CellAddress;
using ss::core::DependencyGraph;
using ss::core::RecalcPlan;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

std::set<CellAddress> cells(std::initializer_list<std::string_view> texts) {
    std::set<CellAddress> result;
    for (const std::string_view text : texts) {
        result.insert(at(text));
    }
    return result;
}

std::vector<CellAddress> list(std::initializer_list<std::string_view> texts) {
    std::vector<CellAddress> result;
    for (const std::string_view text : texts) {
        result.push_back(at(text));
    }
    return result;
}

// Граф строим без формул: "B1 зависит от A1" задаём руками.
class Graph : public ::testing::Test {
protected:
    void depends(std::string_view cell, std::initializer_list<std::string_view> on) {
        graph.set_dependencies(at(cell), cells(on));
    }

    RecalcPlan plan_for(std::initializer_list<std::string_view> changed) {
        return graph.plan(list(changed));
    }

    DependencyGraph graph;
};

}  // namespace

TEST_F(Graph, ChainIsRecalculatedInOrder) {
    depends("B1", {"A1"});
    depends("C1", {"B1"});
    const RecalcPlan plan = plan_for({"A1"});
    EXPECT_EQ(plan.order, list({"A1", "B1", "C1"}));
    EXPECT_TRUE(plan.cyclic.empty());
}

TEST_F(Graph, ChainOrderDoesNotDependOnAddresses) {
    // зависимость идёт справа налево, сортировка по адресу тут не поможет
    depends("A1", {"B1"});
    depends("B1", {"C1"});
    EXPECT_EQ(plan_for({"C1"}).order, list({"C1", "B1", "A1"}));
}

TEST_F(Graph, DiamondTopIsRecalculatedOnceAndLast) {
    depends("B1", {"A1"});
    depends("C1", {"A1"});
    depends("D1", {"B1", "C1"});
    const RecalcPlan plan = plan_for({"A1"});
    ASSERT_EQ(plan.order.size(), 4U);
    EXPECT_EQ(plan.order.front(), at("A1"));
    EXPECT_EQ(plan.order.back(), at("D1"));
    EXPECT_EQ(std::count(plan.order.begin(), plan.order.end(), at("D1")), 1);
    EXPECT_TRUE(plan.cyclic.empty());
}

TEST_F(Graph, UnrelatedCellsAreNotPlanned) {
    depends("B1", {"A1"});
    depends("D1", {"C1"});
    EXPECT_EQ(plan_for({"A1"}).order, list({"A1", "B1"}));
}

TEST_F(Graph, DependenciesOfChangedCellAreNotPlanned) {
    depends("B1", {"A1"});
    EXPECT_EQ(plan_for({"B1"}).order, list({"B1"}));
}

TEST_F(Graph, SeveralChangedCells) {
    depends("B1", {"A1"});
    depends("D1", {"C1"});
    depends("E1", {"B1", "D1"});
    const RecalcPlan plan = plan_for({"A1", "C1"});
    EXPECT_EQ(plan.order.size(), 5U);
    EXPECT_EQ(plan.order.back(), at("E1"));
}

TEST_F(Graph, CellWithoutEdges) {
    EXPECT_EQ(plan_for({"Z9"}).order, list({"Z9"}));
}

TEST_F(Graph, SelfReferenceIsCycle) {
    depends("A1", {"A1"});
    const RecalcPlan plan = plan_for({"A1"});
    EXPECT_TRUE(plan.order.empty());
    EXPECT_EQ(plan.cyclic, cells({"A1"}));
}

TEST_F(Graph, CycleOfThree) {
    depends("A1", {"B1"});
    depends("B1", {"C1"});
    depends("C1", {"A1"});
    const RecalcPlan plan = plan_for({"B1"});
    EXPECT_TRUE(plan.order.empty());
    EXPECT_EQ(plan.cyclic, cells({"A1", "B1", "C1"}));
}

TEST_F(Graph, CellsBelowCycleAreCyclicToo) {
    depends("A1", {"B1"});
    depends("B1", {"A1"});
    depends("C1", {"A1"});
    depends("D1", {"C1"});
    const RecalcPlan plan = plan_for({"A1"});
    EXPECT_EQ(plan.cyclic, cells({"A1", "B1", "C1", "D1"}));
}

TEST_F(Graph, CellsAboveCycleAreNotCyclic) {
    // X1 только питает цикл, сама в нём не участвует
    depends("A1", {"B1", "X1"});
    depends("B1", {"A1"});
    const RecalcPlan plan = plan_for({"X1"});
    EXPECT_EQ(plan.order, list({"X1"}));
    EXPECT_EQ(plan.cyclic, cells({"A1", "B1"}));
}

TEST_F(Graph, BreakingCycleRestoresOrder) {
    depends("A1", {"B1"});
    depends("B1", {"A1"});
    depends("C1", {"A1"});
    graph.set_dependencies(at("B1"), {});  // в B1 теперь просто число
    const RecalcPlan plan = plan_for({"B1"});
    EXPECT_EQ(plan.order, list({"B1", "A1", "C1"}));
    EXPECT_TRUE(plan.cyclic.empty());
}

TEST_F(Graph, NewDependenciesReplaceOldOnes) {
    depends("B1", {"A1", "A2"});
    depends("B1", {"A2", "A3"});
    EXPECT_EQ(graph.dependencies_of(at("B1")), cells({"A2", "A3"}));
    EXPECT_TRUE(graph.dependents_of(at("A1")).empty());
    EXPECT_EQ(graph.dependents_of(at("A2")), cells({"B1"}));
    EXPECT_EQ(graph.dependents_of(at("A3")), cells({"B1"}));
    EXPECT_EQ(plan_for({"A1"}).order, list({"A1"}));
}

TEST_F(Graph, RemoveCellDropsItsEdges) {
    depends("B1", {"A1"});
    depends("C1", {"B1"});
    graph.remove_cell(at("B1"));
    EXPECT_TRUE(graph.dependencies_of(at("B1")).empty());
    EXPECT_TRUE(graph.dependents_of(at("A1")).empty());
    // C1 по-прежнему ссылается на B1, хоть она и пустая
    EXPECT_EQ(graph.dependents_of(at("B1")), cells({"C1"}));
    EXPECT_EQ(plan_for({"A1"}).order, list({"A1"}));
}

TEST_F(Graph, RemoveUnknownCellDoesNothing) {
    graph.remove_cell(at("Q7"));
    EXPECT_TRUE(graph.dependents_of(at("Q7")).empty());
}
