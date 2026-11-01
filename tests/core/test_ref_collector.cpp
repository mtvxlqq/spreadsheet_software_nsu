#include <initializer_list>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/errors.hpp"
#include "core/expr.hpp"
#include "core/ref_collector.hpp"
#include "support/expr_builders.hpp"
#include "support/printers.hpp"

using ss::core::BinaryOp;
using ss::core::CellAddress;
using ss::core::ExprPtr;
using ss::core::LimitError;
using ss::core::RefCollector;
using namespace ss::test;

namespace {

std::set<CellAddress> collect(const ExprPtr& expr) {
    return RefCollector::collect(*expr);
}

std::set<CellAddress> addresses(std::initializer_list<std::string_view> texts) {
    std::set<CellAddress> result;
    for (const std::string_view text : texts) {
        result.insert(CellAddress::parse(text));
    }
    return result;
}

}  // namespace

TEST(RefCollector, CollectsSingleReference) {
    EXPECT_EQ(collect(cell("B2")), addresses({"B2"}));
    EXPECT_EQ(collect(cell("$B$2")), addresses({"B2"}));
}

TEST(RefCollector, ExpandsRangeIncludingEmptyCells) {
    EXPECT_EQ(collect(range_of("A1", "B2")), addresses({"A1", "B1", "A2", "B2"}));
    EXPECT_EQ(collect(range_of("B2", "A1")), addresses({"A1", "B1", "A2", "B2"}));
}

TEST(RefCollector, WalksIntoOperatorsAndFunctions) {
    using enum BinaryOp;
    // =SUM(A1:A3)+IF(B1>0, C1, -D1): берём обе ветки IF,
    // заранее ведь неизвестно, какая из них посчитается
    const ExprPtr expr = bin(Add, call("SUM", range_of("A1", "A3")),
                             call("IF", bin(Gt, cell("B1"), num(0)), cell("C1"), neg(cell("D1"))));
    EXPECT_EQ(collect(expr), addresses({"A1", "A2", "A3", "B1", "C1", "D1"}));
}

TEST(RefCollector, MergesDuplicates) {
    using enum BinaryOp;
    EXPECT_EQ(collect(bin(Add, cell("A1"), bin(Mul, cell("a1"), cell("$A$1")))), addresses({"A1"}));
}

TEST(RefCollector, LiteralsHaveNoDependencies) {
    EXPECT_TRUE(collect(num(1)).empty());
    EXPECT_TRUE(collect(str("A1")).empty());
    EXPECT_TRUE(collect(call("PI")).empty());
}

TEST(RefCollector, SkipsReferencesOutsideSheet) {
    using enum BinaryOp;
    EXPECT_EQ(collect(bin(Add, cell("A1"), cell("A99999999"))), addresses({"A1"}));
    EXPECT_TRUE(collect(range_of("A1", "AAA1")).empty());
}

TEST(RefCollector, AcceptsWholeColumn) {
    EXPECT_EQ(collect(range_of("A1", "A65536")).size(), 65536U);
}

TEST(RefCollector, RejectsHugeRanges) {
    EXPECT_THROW(collect(call("SUM", range_of("A1", "ZZ65536"))), LimitError);
    // предел на всю формулу, а не на один диапазон: четыре столбца уже много
    EXPECT_THROW(collect(call("SUM", range_of("A1", "A65536"), range_of("B1", "B65536"),
                              range_of("C1", "C65536"), range_of("D1", "D65536"))),
                 LimitError);
}
