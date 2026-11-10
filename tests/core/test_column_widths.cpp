#include <map>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/column_widths.hpp"
#include "core/errors.hpp"

using ss::core::ColumnWidths;

TEST(ColumnWidthsTest, DefaultWidthForEveryColumn) {
    const ColumnWidths widths;
    EXPECT_EQ(widths.get(0), ss::core::default_column_width);
    EXPECT_EQ(widths.get(ss::core::max_cols - 1), ss::core::default_column_width);
    EXPECT_TRUE(widths.custom().empty());
}

TEST(ColumnWidthsTest, SetAndGet) {
    ColumnWidths widths;
    widths.set(2, 15);
    widths.set(0, 3);
    EXPECT_EQ(widths.get(2), 15);
    EXPECT_EQ(widths.get(0), 3);
    EXPECT_EQ(widths.get(1), ss::core::default_column_width);
    EXPECT_EQ(widths.custom(), (std::map<int, int>{{0, 3}, {2, 15}}));
}

// обычную ширину не храним, иначе два одинаковых листа были бы не равны
TEST(ColumnWidthsTest, DefaultWidthIsNotStored) {
    ColumnWidths widths;
    widths.set(2, 15);
    widths.set(2, ss::core::default_column_width);
    EXPECT_TRUE(widths.custom().empty());
    EXPECT_EQ(widths, ColumnWidths{});
}

TEST(ColumnWidthsTest, LimitsAreChecked) {
    ColumnWidths widths;
    EXPECT_NO_THROW(widths.set(0, ss::core::min_column_width));
    EXPECT_NO_THROW(widths.set(0, ss::core::max_column_width));
    EXPECT_THROW(widths.set(0, ss::core::min_column_width - 1), ss::core::LimitError);
    EXPECT_THROW(widths.set(0, ss::core::max_column_width + 1), ss::core::LimitError);
    EXPECT_THROW(widths.set(0, 0), ss::core::LimitError);
    EXPECT_THROW(widths.set(-1, 10), ss::core::LimitError);
    EXPECT_THROW(widths.set(ss::core::max_cols, 10), ss::core::LimitError);
    // после неудачи ширина прежняя
    EXPECT_EQ(widths.get(0), ss::core::max_column_width);
}
