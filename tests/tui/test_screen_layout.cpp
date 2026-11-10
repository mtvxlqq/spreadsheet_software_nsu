#include <gtest/gtest.h>

#include "tui/screen_layout.hpp"

using ss::tui::compute_layout;
using ss::tui::ScreenLayout;

// размер из задания
TEST(ScreenLayoutTest, Standard80x24) {
    EXPECT_EQ(compute_layout(80, 24), (ScreenLayout{.visible_rows = 21, .grid_width = 74}));
}

TEST(ScreenLayoutTest, RowNumbersTakeSixPositions) {
    EXPECT_EQ(compute_layout(26, 24).grid_width, 20);
    EXPECT_EQ(compute_layout(7, 24).grid_width, 1);
}

TEST(ScreenLayoutTest, BigWindow) {
    EXPECT_EQ(compute_layout(206, 53), (ScreenLayout{.visible_rows = 50, .grid_width = 200}));
}

TEST(ScreenLayoutTest, TinyWindowStillHasOneCell) {
    EXPECT_EQ(compute_layout(1, 1), (ScreenLayout{.visible_rows = 1, .grid_width = 1}));
}

TEST(ScreenLayoutTest, ZeroAndNegativeSizes) {
    EXPECT_EQ(compute_layout(0, 0), (ScreenLayout{.visible_rows = 1, .grid_width = 1}));
    EXPECT_EQ(compute_layout(-5, -5), (ScreenLayout{.visible_rows = 1, .grid_width = 1}));
}
