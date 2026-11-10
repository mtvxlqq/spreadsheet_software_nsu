#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "app/structure_command.hpp"
#include "core/errors.hpp"
#include "core/ref_mapping.hpp"
#include "support/printers.hpp"
#include "support/session_fixture.hpp"

using ss::test::at;

namespace {

class StructureTest : public ss::test::SessionTest {
protected:
    // столбец A с числами 1..count и суммой под ними
    void fill_column(int count) {
        for (int row = 1; row <= count; ++row) {
            input("A" + std::to_string(row), std::to_string(row));
        }
        input("A" + std::to_string(count + 1), "=SUM(A1:A" + std::to_string(count) + ")");
    }
};

}  // namespace

TEST_F(StructureTest, F5InsertsRowAboveCursor) {
    fill_column(3);
    go("A2");
    press({"f5"});
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(raw("A1"), "1");
    EXPECT_EQ(raw("A2"), "");
    EXPECT_EQ(raw("A3"), "2");
    EXPECT_EQ(raw("A5"), "=SUM(A1:A4)");
    EXPECT_EQ(shown("A5"), "6");
    EXPECT_EQ(session.cursor(), at("A2"));
    EXPECT_TRUE(session.dirty());
}

TEST_F(StructureTest, F6DeletesRowUnderCursor) {
    fill_column(3);
    go("B2");
    press({"f6"});
    EXPECT_EQ(raw("A2"), "3");
    EXPECT_EQ(raw("A3"), "=SUM(A1:A2)");
    EXPECT_EQ(shown("A3"), "4");
}

TEST_F(StructureTest, F7AndF8WorkWithColumns) {
    input("A1", "1");
    input("B1", "=A1+1");
    go("B5");
    press({"f7"});
    EXPECT_EQ(raw("B1"), "");
    EXPECT_EQ(raw("C1"), "=A1+1");
    go("A1");
    press({"f8"});
    EXPECT_EQ(raw("B1"), "=#REF!+1");
    EXPECT_EQ(shown("B1"), "#REF!");
}

TEST_F(StructureTest, CommandTakesCount) {
    fill_column(3);
    go("A2");
    command("ir 3");
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(raw("A5"), "2");
    EXPECT_EQ(raw("A7"), "=SUM(A1:A6)");
    command("dr 3");
    EXPECT_EQ(raw("A2"), "2");
    EXPECT_EQ(raw("A4"), "=SUM(A1:A3)");

    go("A1");
    command("ic 2");
    EXPECT_EQ(raw("C4"), "=SUM(C1:C3)");
    command("dc 2");
    EXPECT_EQ(raw("A4"), "=SUM(A1:A3)");
}

TEST_F(StructureTest, FullNamesWorkToo) {
    input("A1", "x");
    go("A1");
    command("insert-row");
    EXPECT_EQ(raw("A2"), "x");
    command("delete-row");
    EXPECT_EQ(raw("A1"), "x");
    command("insert-col");
    EXPECT_EQ(raw("B1"), "x");
    command("delete-col");
    EXPECT_EQ(raw("A1"), "x");
}

// без числа берётся столько строк, сколько выделено
TEST_F(StructureTest, SelectionGivesCount) {
    fill_column(5);
    go("A2");
    press({"f3", "down", "down", "f6"});
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_EQ(raw("A1"), "1");
    EXPECT_EQ(raw("A2"), "5");
    EXPECT_EQ(raw("A3"), "=SUM(A1:A2)");

    go("C1");
    press({"f3", "right", "f7"});
    EXPECT_EQ(session.document().used_range()->to().col, 0);
    input("C1", "c");
    go("B1");
    press({"f3", "left", "f7"});
    EXPECT_EQ(raw("E1"), "c");
}

TEST_F(StructureTest, DeletedRowsComeBackOnUndo) {
    fill_column(3);
    input("B2", "=A2*10");
    go("A2");
    press({"f6"});
    EXPECT_EQ(raw("B2"), "");
    EXPECT_EQ(raw("A3"), "=SUM(A1:A2)");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A2"), "2");
    EXPECT_EQ(raw("B2"), "=A2*10");
    EXPECT_EQ(raw("A4"), "=SUM(A1:A3)");
    EXPECT_EQ(shown("A4"), "6");
    EXPECT_EQ(shown("B2"), "20");
    press({"ctrl+y"});
    EXPECT_EQ(raw("A3"), "=SUM(A1:A2)");
}

// после удаления в формуле #REF!, а отмена возвращает настоящую ссылку
TEST_F(StructureTest, UndoRestoresReferenceInsteadOfRefError) {
    input("A1", "=A2+1");
    input("A2", "5");
    go("A2");
    press({"f6"});
    EXPECT_EQ(raw("A1"), "=#REF!+1");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "=A2+1");
    EXPECT_EQ(shown("A1"), "6");
}

TEST_F(StructureTest, InsertPastSheetEdgeIsRejected) {
    input("A65536", "last");
    go("A1");
    press({"f5"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(raw("A65536"), "last");
    // в историю не попало: отменяется сам ввод
    press({"ctrl+z"});
    EXPECT_EQ(raw("A65536"), "");
}

TEST_F(StructureTest, BadArgumentsAreReported) {
    input("A1", "x");
    go("A1");
    for (const char* line : {"ir abc", "ir 0", "ir -2", "ir 1 2", "dr 1.5", "ic 99999", "dc 0"}) {
        command(line);
        EXPECT_TRUE(failed()) << line;
    }
    press({"esc"});
    EXPECT_EQ(raw("A1"), "x");
}

TEST_F(StructureTest, StylesAndWidthsFollowTheirCells) {
    input("B2", "0.5");
    go("B2");
    command("format percent 0");
    command("width 20");
    go("A1");
    press({"f5", "f7"});
    EXPECT_EQ(raw("C3"), "0.5");
    EXPECT_EQ(session.document().style(at("C3")).format, "percent:0");
    EXPECT_EQ(session.document().style(at("B2")), ss::core::CellStyle{});
    EXPECT_EQ(session.document().column_widths().get(2), 20);
    EXPECT_EQ(session.document().column_widths().get(1), 10);
    press({"ctrl+z", "ctrl+z"});
    EXPECT_EQ(session.document().style(at("B2")).format, "percent:0");
    EXPECT_EQ(session.document().column_widths().get(1), 20);
}

TEST_F(StructureTest, CommandCanBeUsedDirectly) {
    input("A1", "1");
    input("A2", "=A1+1");
    session.apply_edit(std::make_unique<ss::app::StructureCommand>(
        ss::core::StructureEdit{.kind = ss::core::StructureEdit::Kind::Insert,
                                .axis = ss::core::Axis::Row,
                                .at = 0,
                                .count = 2},
        functions));
    EXPECT_EQ(raw("A4"), "=A3+1");
    EXPECT_TRUE(session.undo());
    EXPECT_EQ(raw("A2"), "=A1+1");
}
