#include <gtest/gtest.h>

#include "app/viewport.hpp"
#include "core/address.hpp"

using ss::app::Viewport;
using ss::core::CellAddress;

TEST(ViewportTest, StartsAtTopLeft) {
    Viewport viewport;
    viewport.resize(10, 5);
    EXPECT_EQ(viewport.first_row(), 0);
    EXPECT_EQ(viewport.first_col(), 0);
    EXPECT_TRUE(viewport.contains({.row = 9, .col = 4}));
    EXPECT_FALSE(viewport.contains({.row = 10, .col = 4}));
    EXPECT_FALSE(viewport.contains({.row = 9, .col = 5}));
}

TEST(ViewportTest, ScrollsMinimallyDownAndRight) {
    Viewport viewport;
    viewport.resize(10, 5);
    viewport.ensure_visible({.row = 10, .col = 5});
    EXPECT_EQ(viewport.first_row(), 1);
    EXPECT_EQ(viewport.first_col(), 1);
    viewport.ensure_visible({.row = 25, .col = 2});
    EXPECT_EQ(viewport.first_row(), 16);
    EXPECT_EQ(viewport.first_col(), 1);
}

TEST(ViewportTest, ScrollsBackUpAndLeft) {
    Viewport viewport;
    viewport.resize(10, 5);
    viewport.ensure_visible({.row = 50, .col = 50});
    viewport.ensure_visible({.row = 45, .col = 46});
    EXPECT_EQ(viewport.first_row(), 41);
    EXPECT_EQ(viewport.first_col(), 46);
    viewport.ensure_visible({.row = 0, .col = 0});
    EXPECT_EQ(viewport.first_row(), 0);
    EXPECT_EQ(viewport.first_col(), 0);
}

TEST(ViewportTest, VisibleCellDoesNotScroll) {
    Viewport viewport;
    viewport.resize(10, 5);
    viewport.ensure_visible({.row = 30, .col = 0});
    viewport.ensure_visible({.row = 25, .col = 3});
    EXPECT_EQ(viewport.first_row(), 21);
    EXPECT_EQ(viewport.first_col(), 0);
}

TEST(ViewportTest, DoesNotGoPastSheetEnd) {
    Viewport viewport;
    viewport.resize(10, 5);
    viewport.ensure_visible({.row = ss::core::max_rows - 1, .col = ss::core::max_cols - 1});
    EXPECT_EQ(viewport.first_row(), ss::core::max_rows - 10);
    EXPECT_EQ(viewport.first_col(), ss::core::max_cols - 5);
    // экран вырос, окно сдвигается назад, а не торчит за лист
    viewport.resize(20, 10);
    viewport.ensure_visible({.row = ss::core::max_rows - 1, .col = ss::core::max_cols - 1});
    EXPECT_EQ(viewport.first_row(), ss::core::max_rows - 20);
    EXPECT_EQ(viewport.first_col(), ss::core::max_cols - 10);
}

TEST(ViewportTest, TinyOrNegativeSizeBecomesOne) {
    Viewport viewport;
    viewport.resize(0, -3);
    EXPECT_EQ(viewport.visible_rows(), 1);
    EXPECT_EQ(viewport.visible_cols(), 1);
    viewport.ensure_visible({.row = 7, .col = 3});
    EXPECT_EQ(viewport.first_row(), 7);
    EXPECT_EQ(viewport.first_col(), 3);
}
