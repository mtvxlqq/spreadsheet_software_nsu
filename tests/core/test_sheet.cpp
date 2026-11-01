#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/cell.hpp"
#include "core/errors.hpp"
#include "core/sheet.hpp"
#include "core/value.hpp"
#include "support/printers.hpp"

using ss::core::Cell;
using ss::core::CellAddress;
using ss::core::Range;
using ss::core::Sheet;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

Cell number_cell(double value) {
    return Cell{.raw = "x", .formula = nullptr, .value = ss::core::make_number(value)};
}

}  // namespace

TEST(Sheet, EmptyCellIsNull) {
    const Sheet sheet;
    EXPECT_EQ(sheet.get(at("A1")), nullptr);
    EXPECT_EQ(sheet.size(), 0U);
    EXPECT_EQ(sheet.used_range(), std::nullopt);
}

TEST(Sheet, SetGetRemove) {
    Sheet sheet;
    sheet.set(at("B2"), number_cell(5));
    ASSERT_NE(sheet.get(at("B2")), nullptr);
    EXPECT_EQ(sheet.get(at("B2"))->value->as_number(), 5);

    sheet.set(at("B2"), number_cell(7));
    EXPECT_EQ(sheet.get(at("B2"))->value->as_number(), 7);
    EXPECT_EQ(sheet.size(), 1U);

    sheet.remove(at("B2"));
    EXPECT_EQ(sheet.get(at("B2")), nullptr);
    sheet.remove(at("B2"));  // повторное удаление ничего не ломает
}

TEST(Sheet, SetValueChangesOnlyExistingCell) {
    Sheet sheet;
    sheet.set(at("A1"), number_cell(1));
    sheet.set_value(at("A1"), ss::core::make_number(2));
    sheet.set_value(at("C3"), ss::core::make_number(3));
    EXPECT_EQ(sheet.get(at("A1"))->value->as_number(), 2);
    EXPECT_EQ(sheet.get(at("C3")), nullptr);
}

TEST(Sheet, RejectsAddressOutsideSheet) {
    Sheet sheet;
    EXPECT_THROW(sheet.set(CellAddress{.row = ss::core::max_rows, .col = 0}, number_cell(1)),
                 ss::core::LimitError);
    EXPECT_EQ(sheet.size(), 0U);
}

TEST(Sheet, UsedRangeCoversAllCells) {
    Sheet sheet;
    sheet.set(at("C2"), number_cell(1));
    sheet.set(at("B5"), number_cell(1));
    sheet.set(at("E3"), number_cell(1));
    EXPECT_EQ(sheet.used_range(), Range(at("B2"), at("E5")));
}

TEST(Sheet, ForEachGoesByRows) {
    Sheet sheet;
    for (const std::string_view text : {"B2", "A2", "C1", "A1"}) {
        sheet.set(at(text), number_cell(1));
    }
    std::vector<std::string> visited;
    sheet.for_each([&](CellAddress address, const Cell& /*cell*/) {
        visited.push_back(address.to_string());
    });
    EXPECT_EQ(visited, (std::vector<std::string>{"A1", "C1", "A2", "B2"}));
}
