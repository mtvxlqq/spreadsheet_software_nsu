#include <gtest/gtest.h>

#include "app/viewport.hpp"
#include "core/address.hpp"
#include "core/column_widths.hpp"

using ss::app::Viewport;
using ss::core::CellAddress;
using ss::core::ColumnWidths;

namespace {

// все столбцы по 10 позиций, как у нового листа
const ColumnWidths plain;

}  // namespace

TEST(ViewportTest, StartsAtTopLeft) {
    Viewport viewport;
    viewport.resize(10, 50);
    EXPECT_EQ(viewport.first_row(), 0);
    EXPECT_EQ(viewport.first_col(), 0);
    EXPECT_EQ(viewport.visible_cols(plain), 5);
    EXPECT_TRUE(viewport.contains({.row = 9, .col = 4}, plain));
    EXPECT_FALSE(viewport.contains({.row = 10, .col = 4}, plain));
    EXPECT_FALSE(viewport.contains({.row = 9, .col = 5}, plain));
}

TEST(ViewportTest, ScrollsMinimallyDownAndRight) {
    Viewport viewport;
    viewport.resize(10, 50);
    viewport.ensure_visible({.row = 10, .col = 5}, plain);
    EXPECT_EQ(viewport.first_row(), 1);
    EXPECT_EQ(viewport.first_col(), 1);
    viewport.ensure_visible({.row = 25, .col = 2}, plain);
    EXPECT_EQ(viewport.first_row(), 16);
    EXPECT_EQ(viewport.first_col(), 1);
}

TEST(ViewportTest, ScrollsBackUpAndLeft) {
    Viewport viewport;
    viewport.resize(10, 50);
    viewport.ensure_visible({.row = 50, .col = 50}, plain);
    viewport.ensure_visible({.row = 45, .col = 46}, plain);
    EXPECT_EQ(viewport.first_row(), 41);
    EXPECT_EQ(viewport.first_col(), 46);
    viewport.ensure_visible({.row = 0, .col = 0}, plain);
    EXPECT_EQ(viewport.first_row(), 0);
    EXPECT_EQ(viewport.first_col(), 0);
}

TEST(ViewportTest, VisibleCellDoesNotScroll) {
    Viewport viewport;
    viewport.resize(10, 50);
    viewport.ensure_visible({.row = 30, .col = 0}, plain);
    viewport.ensure_visible({.row = 25, .col = 3}, plain);
    EXPECT_EQ(viewport.first_row(), 21);
    EXPECT_EQ(viewport.first_col(), 0);
}

TEST(ViewportTest, DoesNotGoPastSheetEnd) {
    Viewport viewport;
    viewport.resize(10, 50);
    viewport.ensure_visible({.row = ss::core::max_rows - 1, .col = ss::core::max_cols - 1}, plain);
    EXPECT_EQ(viewport.first_row(), ss::core::max_rows - 10);
    EXPECT_EQ(viewport.first_col(), ss::core::max_cols - 5);
    // экран вырос, окно сдвигается назад, а не торчит за лист
    viewport.resize(20, 100);
    viewport.ensure_visible({.row = ss::core::max_rows - 1, .col = ss::core::max_cols - 1}, plain);
    EXPECT_EQ(viewport.first_row(), ss::core::max_rows - 20);
    EXPECT_EQ(viewport.first_col(), ss::core::max_cols - 10);
    EXPECT_EQ(viewport.visible_cols(plain), 10);
}

TEST(ViewportTest, TinyOrNegativeSizeBecomesOne) {
    Viewport viewport;
    viewport.resize(0, -3);
    EXPECT_EQ(viewport.visible_rows(), 1);
    EXPECT_EQ(viewport.width(), 1);
    // столбец шире окна, но один всё равно считается видимым
    EXPECT_EQ(viewport.visible_cols(plain), 1);
    viewport.ensure_visible({.row = 7, .col = 3}, plain);
    EXPECT_EQ(viewport.first_row(), 7);
    EXPECT_EQ(viewport.first_col(), 3);
}

// размер из задания: 80 позиций минус 6 на номера строк
TEST(ViewportTest, StandardWindowWithDifferentWidths) {
    Viewport viewport;
    viewport.resize(21, 74);
    EXPECT_EQ(viewport.visible_cols(plain), 7);

    ColumnWidths widths;
    widths.set(0, 30);
    widths.set(1, 3);
    // 30 + 3 + 10 + 10 + 10 + 10 = 73, седьмой столбец уже не влезает
    EXPECT_EQ(viewport.visible_cols(widths), 6);

    widths.set(0, 60);
    EXPECT_EQ(viewport.visible_cols(widths), 3);
}

TEST(ViewportTest, PartlyVisibleColumnIsNotCounted) {
    Viewport viewport;
    viewport.resize(5, 19);
    EXPECT_EQ(viewport.visible_cols(plain), 1);
    viewport.resize(5, 20);
    EXPECT_EQ(viewport.visible_cols(plain), 2);
}

TEST(ViewportTest, CursorColumnIsFullyVisibleAfterScrollRight) {
    ColumnWidths widths;
    widths.set(2, 40);
    Viewport viewport;
    viewport.resize(5, 50);
    // A, B и C это 10 + 10 + 40, в 50 не влезают, остаются B и C
    viewport.ensure_visible({.row = 0, .col = 2}, widths);
    EXPECT_EQ(viewport.first_col(), 1);
    EXPECT_TRUE(viewport.contains({.row = 0, .col = 2}, widths));
    // D рядом с широким C не помещается, C уходит за левый край
    viewport.ensure_visible({.row = 0, .col = 3}, widths);
    EXPECT_EQ(viewport.first_col(), 2);
    EXPECT_TRUE(viewport.contains({.row = 0, .col = 3}, widths));
}

TEST(ViewportTest, ScrollLeftShowsCursorColumnFirst) {
    ColumnWidths widths;
    widths.set(0, 40);
    Viewport viewport;
    viewport.resize(5, 50);
    viewport.ensure_visible({.row = 0, .col = 10}, widths);
    EXPECT_EQ(viewport.first_col(), 6);
    viewport.ensure_visible({.row = 0, .col = 0}, widths);
    EXPECT_EQ(viewport.first_col(), 0);
    // широкий A и ещё один столбец
    EXPECT_EQ(viewport.visible_cols(widths), 2);
}

TEST(ViewportTest, ColumnWiderThanWindowIsStillShown) {
    ColumnWidths widths;
    widths.set(3, 60);
    Viewport viewport;
    viewport.resize(5, 30);
    viewport.ensure_visible({.row = 0, .col = 3}, widths);
    EXPECT_EQ(viewport.first_col(), 3);
    EXPECT_EQ(viewport.visible_cols(widths), 1);
    EXPECT_TRUE(viewport.contains({.row = 0, .col = 3}, widths));
    // и с него можно уйти дальше
    viewport.ensure_visible({.row = 0, .col = 4}, widths);
    EXPECT_EQ(viewport.first_col(), 4);
}

// столбец стал шире, и курсор справа от него перестал помещаться
TEST(ViewportTest, WideningColumnScrollsToKeepCursor) {
    ColumnWidths widths;
    Viewport viewport;
    viewport.resize(5, 50);
    viewport.ensure_visible({.row = 0, .col = 4}, widths);
    EXPECT_EQ(viewport.first_col(), 0);
    widths.set(1, 25);
    EXPECT_FALSE(viewport.contains({.row = 0, .col = 4}, widths));
    viewport.ensure_visible({.row = 0, .col = 4}, widths);
    EXPECT_TRUE(viewport.contains({.row = 0, .col = 4}, widths));
    EXPECT_EQ(viewport.first_col(), 2);
}
