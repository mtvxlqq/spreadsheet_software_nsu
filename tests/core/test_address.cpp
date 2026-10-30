#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/errors.hpp"
#include "support/printers.hpp"

using ss::core::AddressError;
using ss::core::CellAddress;
using ss::core::CellRef;
using ss::core::columnName;
using ss::core::Range;

TEST(CellAddress, ParsesLettersAndDigits) {
    EXPECT_EQ(CellAddress::parse("A1"), (CellAddress{0, 0}));
    EXPECT_EQ(CellAddress::parse("Z1"), (CellAddress{0, 25}));
    EXPECT_EQ(CellAddress::parse("AA1"), (CellAddress{0, 26}));
    EXPECT_EQ(CellAddress::parse("B12"), (CellAddress{11, 1}));
    EXPECT_EQ(CellAddress::parse("ZZ65536"), (CellAddress{65535, 701}));
}

TEST(CellAddress, ParsingIgnoresCase) {
    EXPECT_EQ(CellAddress::parse("b12"), CellAddress::parse("B12"));
    EXPECT_EQ(CellAddress::parse("aZ3"), CellAddress::parse("AZ3"));
}

TEST(CellAddress, ToStringIsInverseOfParse) {
    for (const std::string text : {"A1", "Z1", "AA1", "AZ10", "BA7", "ZZ65536"}) {
        EXPECT_EQ(CellAddress::parse(text).toString(), text);
    }
}

TEST(CellAddress, RejectsMalformedText) {
    for (const std::string text : {"", "A", "1", "1A", "A1B", "A-1", "$A$1", " A1", "A 1", "Ж1"}) {
        EXPECT_THROW(CellAddress::parse(text), AddressError) << text;
    }
}

TEST(CellAddress, RejectsAddressesOutsideSheet) {
    for (const std::string text : {"A0", "AAA1", "A65537", "ZZZ99999999"}) {
        EXPECT_THROW(CellAddress::parse(text), AddressError) << text;
    }
}

TEST(CellAddress, ValidityFollowsSheetLimits) {
    EXPECT_TRUE((CellAddress{0, 0}).isValid());
    EXPECT_TRUE((CellAddress{65535, 701}).isValid());
    EXPECT_FALSE((CellAddress{65536, 0}).isValid());
    EXPECT_FALSE((CellAddress{0, 702}).isValid());
    EXPECT_FALSE((CellAddress{-1, 0}).isValid());
}

TEST(CellAddress, ColumnNames) {
    EXPECT_EQ(columnName(0), "A");
    EXPECT_EQ(columnName(25), "Z");
    EXPECT_EQ(columnName(26), "AA");
    EXPECT_EQ(columnName(701), "ZZ");
    EXPECT_EQ(columnName(702), "AAA");
}

TEST(CellAddress, OrderedByRowThenColumn) {
    const CellAddress a1 = CellAddress::parse("A1");
    const CellAddress b1 = CellAddress::parse("B1");
    const CellAddress a2 = CellAddress::parse("A2");
    EXPECT_LT(a1, b1);
    EXPECT_LT(b1, a2);
}

TEST(CellAddress, HashDistinguishesRowAndColumn) {
    const std::unordered_set<CellAddress> set = {{0, 1}, {1, 0}, {0, 0}, {1, 1}, {0, 1}};
    EXPECT_EQ(set.size(), 4U);
}

TEST(CellRef, ParsesAbsoluteMarkers) {
    const auto plain = CellRef::tryParse("A1");
    ASSERT_TRUE(plain.has_value());
    EXPECT_FALSE(plain->absCol);
    EXPECT_FALSE(plain->absRow);

    const auto both = CellRef::tryParse("$B$2");
    ASSERT_TRUE(both.has_value());
    EXPECT_EQ(both->addr, (CellAddress{1, 1}));
    EXPECT_TRUE(both->absCol);
    EXPECT_TRUE(both->absRow);

    const auto rowOnly = CellRef::tryParse("a$1");
    ASSERT_TRUE(rowOnly.has_value());
    EXPECT_FALSE(rowOnly->absCol);
    EXPECT_TRUE(rowOnly->absRow);

    const auto colOnly = CellRef::tryParse("$A1");
    ASSERT_TRUE(colOnly.has_value());
    EXPECT_TRUE(colOnly->absCol);
    EXPECT_FALSE(colOnly->absRow);
}

TEST(CellRef, KeepsReferencesOutsideSheet) {
    for (const std::string text : {"A99999999", "AAA1", "A0"}) {
        const auto ref = CellRef::tryParse(text);
        ASSERT_TRUE(ref.has_value()) << text;
        EXPECT_FALSE(ref->addr.isValid()) << text;
    }
}

TEST(CellRef, RejectsTextThatIsNotReference) {
    for (const std::string text : {"", "$", "A", "1", "1A", "A1$", "$$A1", "A1:B2", "SUM"}) {
        EXPECT_FALSE(CellRef::tryParse(text).has_value()) << text;
    }
}

TEST(CellRef, ToStringKeepsDollars) {
    for (const std::string text : {"B7", "$A$1", "A$1", "$A1"}) {
        EXPECT_EQ(CellRef::tryParse(text)->toString(), text);
    }
}

TEST(Range, NormalizesCorners) {
    const Range reversed(CellAddress::parse("B2"), CellAddress::parse("A1"));
    EXPECT_EQ(reversed.from(), CellAddress::parse("A1"));
    EXPECT_EQ(reversed.to(), CellAddress::parse("B2"));

    const Range crossed(CellAddress::parse("A2"), CellAddress::parse("B1"));
    EXPECT_EQ(crossed.from(), CellAddress::parse("A1"));
    EXPECT_EQ(crossed.to(), CellAddress::parse("B2"));
}

TEST(Range, ContainsOnlyCellsInside) {
    const Range range(CellAddress::parse("A1"), CellAddress::parse("B2"));
    EXPECT_TRUE(range.contains(CellAddress::parse("A1")));
    EXPECT_TRUE(range.contains(CellAddress::parse("B2")));
    EXPECT_TRUE(range.contains(CellAddress::parse("A2")));
    EXPECT_FALSE(range.contains(CellAddress::parse("C1")));
    EXPECT_FALSE(range.contains(CellAddress::parse("A3")));
}

TEST(Range, SizeCountsCells) {
    EXPECT_EQ(Range(CellAddress::parse("A1"), CellAddress::parse("B2")).size(), 4U);
    EXPECT_EQ(Range(CellAddress::parse("C3"), CellAddress::parse("C3")).size(), 1U);
    EXPECT_EQ(Range(CellAddress::parse("A1"), CellAddress::parse("C10")).size(), 30U);
}

TEST(Range, ForEachGoesRowByRow) {
    const Range range(CellAddress::parse("B2"), CellAddress::parse("A1"));
    std::vector<std::string> visited;
    range.forEach([&](CellAddress address) { visited.push_back(address.toString()); });
    EXPECT_EQ(visited, (std::vector<std::string>{"A1", "B1", "A2", "B2"}));
}

TEST(Range, ToString) {
    EXPECT_EQ(Range(CellAddress::parse("B2"), CellAddress::parse("A1")).toString(), "A1:B2");
}
