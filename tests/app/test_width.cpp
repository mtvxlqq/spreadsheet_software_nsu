#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "app/set_column_width_command.hpp"
#include "core/column_widths.hpp"
#include "core/errors.hpp"
#include "support/printers.hpp"
#include "support/session_fixture.hpp"

using ss::app::SetColumnWidthCommand;
using ss::test::at;

namespace {

class WidthTest : public ss::test::SessionTest {
protected:
    int width(int col) const {
        return session.document().column_widths().get(col);
    }
};

}  // namespace

TEST_F(WidthTest, KeysMakeColumnWiderAndNarrower) {
    go("B1");
    press({">", ">", ">"});
    EXPECT_EQ(width(1), 13);
    EXPECT_EQ(width(0), 10);
    press({"<"});
    EXPECT_EQ(width(1), 12);
    EXPECT_FALSE(failed());
    EXPECT_TRUE(session.dirty());
}

TEST_F(WidthTest, WidthCommandSetsExactWidth) {
    go("C5");
    command("width 25");
    EXPECT_FALSE(failed());
    EXPECT_EQ(width(2), 25);
    command("width 10");
    EXPECT_TRUE(session.document().column_widths().custom().empty());
}

TEST_F(WidthTest, WidthIsUndoneAndRedone) {
    go("A1");
    command("width 20");
    press({">"});
    EXPECT_EQ(width(0), 21);
    press({"ctrl+z"});
    EXPECT_EQ(width(0), 20);
    press({"ctrl+z"});
    EXPECT_EQ(width(0), 10);
    press({"ctrl+y", "ctrl+y"});
    EXPECT_EQ(width(0), 21);
}

TEST_F(WidthTest, LimitsAreReported) {
    go("A1");
    command("width 2");
    EXPECT_TRUE(failed());
    command("width 61");
    EXPECT_TRUE(failed());
    EXPECT_EQ(width(0), 10);

    command("width 3");
    EXPECT_FALSE(failed());
    press({"<"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(width(0), 3);

    command("width 60");
    press({">"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(width(0), 60);
}

TEST_F(WidthTest, BadArgumentsAreReported) {
    for (const char* line : {"width", "width abc", "width 10 20", "width 1.5", "column-wider 3"}) {
        command(line);
        EXPECT_TRUE(failed()) << line;
    }
    press({"esc"});
    EXPECT_TRUE(session.document().column_widths().custom().empty());
    EXPECT_FALSE(session.dirty());
}

// ширина это оформление: значения и формулы от неё не зависят
TEST_F(WidthTest, WidthDoesNotTouchCells) {
    input("A1", "5");
    input("A2", "=A1*2");
    go("A1");
    command("width 30");
    EXPECT_EQ(raw("A2"), "=A1*2");
    EXPECT_EQ(shown("A2"), "10");
}

TEST_F(WidthTest, CommandSetsSeveralColumnsAtOnce) {
    session.apply_edit(std::make_unique<SetColumnWidthCommand>(1, 3, 7));
    EXPECT_EQ(width(0), 10);
    EXPECT_EQ(width(1), 7);
    EXPECT_EQ(width(3), 7);
    EXPECT_EQ(width(4), 10);
    // отменяется одним шагом
    EXPECT_TRUE(session.undo());
    EXPECT_TRUE(session.document().column_widths().custom().empty());
    EXPECT_FALSE(session.undo());
}

TEST_F(WidthTest, BrokenCommandChangesNothing) {
    EXPECT_THROW(session.apply_edit(std::make_unique<SetColumnWidthCommand>(0, 2, 100)),
                 ss::core::LimitError);
    EXPECT_TRUE(session.document().column_widths().custom().empty());
    EXPECT_FALSE(session.undo());
}

// после смены ширины курсор должен остаться на экране
TEST_F(WidthTest, CursorStaysVisibleWhenColumnGrows) {
    session.resize(20, 50);
    go("E1");
    const ss::core::ColumnWidths& widths = session.document().column_widths();
    EXPECT_EQ(session.viewport().first_col(), 0);
    go("B1");
    command("width 40");
    go("E1");
    EXPECT_TRUE(session.viewport().contains(at("E1"), widths));

    // а теперь шире становится сам столбец под курсором у правого края
    session.set_cursor(at("A1"));
    command("width 30");
    go("B1");
    EXPECT_TRUE(session.viewport().contains(at("B1"), widths));
    press({"ctrl+z", "ctrl+z"});
    EXPECT_TRUE(session.viewport().contains(at("B1"), widths));
}

TEST_F(WidthTest, NewDocumentHasDefaultWidths) {
    go("A1");
    command("width 20");
    session.replace_document(std::make_unique<ss::core::Spreadsheet>(functions), "other.sst");
    EXPECT_EQ(width(0), 10);
    EXPECT_FALSE(session.undo());
}
