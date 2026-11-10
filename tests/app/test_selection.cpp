#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "app/clear_range_command.hpp"
#include "app/clipboard.hpp"
#include "app/paste_command.hpp"
#include "core/address.hpp"
#include "core/cell_style.hpp"
#include "core/errors.hpp"
#include "support/printers.hpp"
#include "support/session_fixture.hpp"

using ss::app::Clipboard;
using ss::app::PasteCommand;
using ss::core::Align;
using ss::core::CellStyle;
using ss::core::Range;
using ss::test::at;

namespace {

class SelectionTest : public ss::test::SessionTest {
protected:
    // выделить диапазон и оставить курсор в его втором углу
    void select(std::string_view from, std::string_view to) {
        if (session.mode_name() != "normal") {
            session.handle_key("esc");
        }
        go(from);
        session.handle_key("f3");
        go(to);
    }

    void copy(std::string_view from, std::string_view to) {
        select(from, to);
        session.handle_key("ctrl+c");
    }

    void paste_at(std::string_view address) {
        go(address);
        session.handle_key("ctrl+v");
    }

    Range range(std::string_view from, std::string_view to) const {
        return {at(from), at(to)};
    }

    const CellStyle& style(std::string_view address) const {
        return session.document().style(at(address));
    }
};

}  // namespace

TEST_F(SelectionTest, F3StartsSelectionAtCursor) {
    go("B2");
    EXPECT_FALSE(session.selection().has_value());
    press({"f3"});
    EXPECT_EQ(session.mode_name(), "select");
    EXPECT_EQ(session.selection(), range("B2", "B2"));
}

TEST_F(SelectionTest, MovesExtendSelection) {
    go("B2");
    press({"f3", "down", "down", "right"});
    EXPECT_EQ(session.selection(), range("B2", "C4"));
    EXPECT_EQ(session.cursor(), at("C4"));
    EXPECT_EQ(session.target_range(), range("B2", "C4"));
    // можно тянуть и в обратную сторону от начала
    press({"left", "left", "up", "up", "up"});
    EXPECT_EQ(session.selection(), range("A1", "B2"));
    EXPECT_EQ(session.mode_name(), "select");
}

TEST_F(SelectionTest, PageAndHomeMovesAlsoExtend) {
    go("C3");
    press({"f3", "pagedown", "home"});
    EXPECT_EQ(session.selection(), range("C3", "A23"));
    press({"ctrl+home"});
    EXPECT_EQ(session.selection(), range("A1", "C3"));
}

TEST_F(SelectionTest, EscDropsSelection) {
    go("B2");
    press({"f3", "down", "esc"});
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_FALSE(session.selection().has_value());
    EXPECT_EQ(session.cursor(), at("B3"));
    EXPECT_EQ(session.target_range(), range("B3", "B3"));
}

TEST_F(SelectionTest, TextKeysDoNotStartEdit) {
    input("A1", "keep");
    go("A1");
    press({"f3", "x", "1", "="});
    EXPECT_EQ(session.mode_name(), "select");
    EXPECT_EQ(raw("A1"), "keep");
}

TEST_F(SelectionTest, F3AgainRestartsFromCursor) {
    go("A1");
    press({"f3", "down", "right", "f3"});
    EXPECT_EQ(session.mode_name(), "select");
    EXPECT_EQ(session.selection(), range("B2", "B2"));
}

// обычное действие работает с диапазоном и снимает выделение
TEST_F(SelectionTest, OrdinaryActionEndsSelection) {
    input("A1", "1");
    input("A2", "2");
    input("A3", "3");
    select("A1", "A2");
    press({"delete"});
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_FALSE(session.selection().has_value());
    EXPECT_EQ(raw("A1"), "");
    EXPECT_EQ(raw("A2"), "");
    EXPECT_EQ(raw("A3"), "3");
}

TEST_F(SelectionTest, ActionMayOpenItsOwnMode) {
    select("A1", "B2");
    press({"enter"});
    EXPECT_EQ(session.mode_name(), "edit");
    EXPECT_FALSE(session.selection().has_value());
    press({"esc"});

    select("A1", "B2");
    press({"?"});
    EXPECT_EQ(session.mode_name(), "help");
    EXPECT_FALSE(session.selection().has_value());
}

TEST_F(SelectionTest, FailedActionKeepsSelection) {
    select("A1", "B2");
    // вставлять пока нечего
    press({"ctrl+v"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(session.mode_name(), "select");
    EXPECT_EQ(session.selection(), range("A1", "B2"));
}

TEST_F(SelectionTest, ClearRangeIsUndoneInOneStep) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("A2", "x");
    select("A1", "B2");
    press({"delete"});
    EXPECT_EQ(raw("B1"), "");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "1");
    EXPECT_EQ(raw("B1"), "=A1+1");
    EXPECT_EQ(raw("A2"), "x");
    EXPECT_EQ(shown("B1"), "2");
    // больше отменять нечего, кроме самих вводов
    press({"ctrl+y"});
    EXPECT_EQ(raw("A2"), "");
}

TEST_F(SelectionTest, ClearKeepsStyles) {
    input("A1", "1.5");
    go("A1");
    command("format fixed 2");
    select("A1", "A2");
    press({"delete"});
    EXPECT_EQ(raw("A1"), "");
    EXPECT_EQ(style("A1").format, "fixed:2");
}

TEST_F(SelectionTest, CommandWorksWithSelectedRange) {
    input("A1", "0.5");
    input("B2", "0.25");
    select("A1", "B2");
    press({":"});
    // пока команда набирается, выделение на месте
    EXPECT_EQ(session.mode_name(), "command");
    EXPECT_EQ(session.selection(), range("A1", "B2"));
    type("format percent 0");
    press({"enter"});
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_FALSE(session.selection().has_value());
    for (const char* address : {"A1", "A2", "B1", "B2"}) {
        EXPECT_EQ(style(address).format, "percent:0") << address;
    }
    EXPECT_EQ(style("C1"), CellStyle{});
    // вся команда отменяется одним шагом
    press({"ctrl+z"});
    EXPECT_EQ(style("B2"), CellStyle{});
}

TEST_F(SelectionTest, BrokenCommandKeepsSelectionAndLine) {
    select("A1", "B2");
    press({":"});
    type("align middle");
    press({"enter"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(session.mode_name(), "command");
    EXPECT_EQ(session.selection(), range("A1", "B2"));
    press({"esc"});
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_FALSE(session.selection().has_value());
}

TEST_F(SelectionTest, WidthKeysChangeAllSelectedColumns) {
    select("B1", "D3");
    press({">", ">"});
    // эти клавиши выделение не снимают
    EXPECT_EQ(session.mode_name(), "select");
    const ss::core::ColumnWidths& widths = session.document().column_widths();
    EXPECT_EQ(widths.get(0), 10);
    EXPECT_EQ(widths.get(1), 12);
    EXPECT_EQ(widths.get(2), 12);
    EXPECT_EQ(widths.get(3), 12);
    EXPECT_EQ(widths.get(4), 10);
}

TEST_F(SelectionTest, CopyRemembersFilledCells) {
    input("A1", "1");
    input("B2", "=A1*2");
    copy("A1", "C3");
    EXPECT_EQ(session.mode_name(), "normal");
    ASSERT_TRUE(session.clipboard().has_value());
    const Clipboard& clipboard = *session.clipboard();
    EXPECT_EQ(clipboard.source, range("A1", "C3"));
    ASSERT_EQ(clipboard.items.size(), 2U);
    EXPECT_EQ(clipboard.items[0].address, at("A1"));
    EXPECT_EQ(clipboard.items[1].content.raw, "=A1*2");
    // копирование документ не меняет: в истории только два ввода
    EXPECT_TRUE(session.undo());
    EXPECT_TRUE(session.undo());
    EXPECT_FALSE(session.undo());
}

TEST_F(SelectionTest, CopyWithoutSelectionTakesCursorCell) {
    input("A1", "7");
    go("A1");
    press({"ctrl+c"});
    paste_at("C3");
    EXPECT_EQ(raw("C3"), "7");
}

TEST_F(SelectionTest, PasteShiftsRelativeReferences) {
    input("A1", "10");
    input("A2", "20");
    input("B1", "=A1*2");
    copy("B1", "B1");
    paste_at("B2");
    EXPECT_EQ(raw("B2"), "=A2*2");
    EXPECT_EQ(shown("B2"), "40");
    paste_at("D5");
    EXPECT_EQ(raw("D5"), "=C5*2");
}

TEST_F(SelectionTest, PasteKeepsAbsoluteParts) {
    input("C1", "=$A$1+A$1+$A1+A1");
    copy("C1", "C1");
    paste_at("D3");
    EXPECT_EQ(raw("D3"), "=$A$1+B$1+$A3+B3");
}

TEST_F(SelectionTest, PasteShiftsRanges) {
    input("C1", "=SUM(A1:$B$2)");
    copy("C1", "C1");
    paste_at("D2");
    EXPECT_EQ(raw("D2"), "=SUM(B2:$B$2)");
}

TEST_F(SelectionTest, ReferenceOutsideSheetBecomesRefError) {
    input("B2", "=A1+1");
    copy("B2", "B2");
    paste_at("A1");
    EXPECT_EQ(raw("A1"), "=#REF!+1");
    EXPECT_EQ(shown("A1"), "#REF!");
}

TEST_F(SelectionTest, PlainValuesAreCopiedAsIs) {
    input("A1", "text");
    input("A2", "'123");
    input("A3", "4.5");
    copy("A1", "A3");
    paste_at("C1");
    EXPECT_EQ(raw("C1"), "text");
    EXPECT_EQ(raw("C2"), "'123");
    EXPECT_EQ(raw("C3"), "4.5");
}

TEST_F(SelectionTest, PasteCopiesStyles) {
    input("A1", "0.5");
    go("A1");
    command("format percent 0");
    command("align center");
    copy("A1", "A1");
    paste_at("B5");
    EXPECT_EQ(style("B5"), (CellStyle{.format = "percent:0", .align = Align::Center}));
}

// в диапазоне были пустые ячейки: на их месте в цели тоже станет пусто
TEST_F(SelectionTest, PasteClearsCellsUnderEmptyOnes) {
    input("A1", "1");
    input("B2", "2");
    input("D1", "old1");
    input("E1", "old2");
    input("D2", "old3");
    input("F1", "outside");
    copy("A1", "B2");
    paste_at("D1");
    EXPECT_EQ(raw("D1"), "1");
    EXPECT_EQ(raw("E1"), "");
    EXPECT_EQ(raw("D2"), "");
    EXPECT_EQ(raw("E2"), "2");
    EXPECT_EQ(raw("F1"), "outside");
}

TEST_F(SelectionTest, PasteIsUndoneInOneStep) {
    input("A1", "1");
    input("A2", "=A1+1");
    input("C1", "old");
    copy("A1", "A2");
    paste_at("C1");
    EXPECT_EQ(raw("C2"), "=C1+1");
    press({"ctrl+z"});
    EXPECT_EQ(raw("C1"), "old");
    EXPECT_EQ(raw("C2"), "");
    press({"ctrl+y"});
    EXPECT_EQ(raw("C1"), "1");
    EXPECT_EQ(shown("C2"), "2");
}

// вставка поверх части исходного диапазона: читаем из буфера, а не из ячеек
TEST_F(SelectionTest, PasteOverlappingSource) {
    input("A1", "1");
    input("A2", "=A1+1");
    input("A3", "=A2+1");
    copy("A1", "A3");
    paste_at("A2");
    EXPECT_EQ(raw("A1"), "1");
    EXPECT_EQ(raw("A2"), "1");
    EXPECT_EQ(raw("A3"), "=A2+1");
    EXPECT_EQ(raw("A4"), "=A3+1");
    EXPECT_EQ(shown("A4"), "3");
}

TEST_F(SelectionTest, ClipboardDoesNotFollowSourceCells) {
    input("A1", "before");
    copy("A1", "A1");
    input("A1", "after");
    go("A1");
    press({"delete"});
    paste_at("B1");
    EXPECT_EQ(raw("B1"), "before");
    // вставлять можно сколько угодно раз
    paste_at("B2");
    EXPECT_EQ(raw("B2"), "before");
}

TEST_F(SelectionTest, ClipboardSurvivesNewDocument) {
    input("A1", "=1+1");
    copy("A1", "A1");
    session.replace_document(std::make_unique<ss::core::Spreadsheet>(functions, value_parsers), "other.sst");
    paste_at("B2");
    EXPECT_EQ(shown("B2"), "2");
}

TEST_F(SelectionTest, PasteOutsideSheetChangesNothing) {
    input("A1", "1");
    input("B2", "2");
    copy("A1", "B2");
    go("ZZ1");
    press({"ctrl+v"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(raw("ZZ1"), "");
    go("A65536");
    press({"ctrl+v"});
    EXPECT_TRUE(failed());
    EXPECT_EQ(raw("A65536"), "");
    // и в историю это не попало: отменяется ввод B2
    press({"ctrl+z"});
    EXPECT_EQ(raw("B2"), "");
}

TEST_F(SelectionTest, PasteWithNothingCopiedIsReported) {
    go("A1");
    press({"ctrl+v"});
    EXPECT_TRUE(failed());
    EXPECT_FALSE(session.dirty());
}

TEST_F(SelectionTest, PasteIntoSelectionUsesItsTopLeftCorner) {
    input("A1", "7");
    copy("A1", "A1");
    select("D4", "C3");
    press({"ctrl+v"});
    EXPECT_EQ(raw("C3"), "7");
    EXPECT_EQ(raw("D4"), "");
    EXPECT_EQ(session.mode_name(), "normal");
}

// сценарий из плана целиком: F3, стрелки, Ctrl+C, стрелки, Ctrl+V, Ctrl+Z
TEST_F(SelectionTest, WholeScenarioByKeys) {
    input("A1", "1");
    input("A2", "2");
    input("B1", "=A1*10");
    input("B2", "=A2*10");
    go("A1");
    press({"f3", "down", "right", "ctrl+c"});
    press({"right", "right", "right", "ctrl+v"});
    EXPECT_EQ(session.cursor(), at("E2"));
    EXPECT_EQ(raw("E2"), "1");
    EXPECT_EQ(raw("F2"), "=E2*10");
    EXPECT_EQ(shown("F3"), "20");
    press({"ctrl+z"});
    EXPECT_EQ(raw("E2"), "");
    EXPECT_EQ(raw("F3"), "");
    EXPECT_EQ(shown("B2"), "20");
}

// весь лист это 46 миллионов ячеек, обходить их по одной нельзя
TEST_F(SelectionTest, HugeSelectionIsFast) {
    input("A1", "1");
    input("ZZ65536", "=A1+1");
    const auto started = std::chrono::steady_clock::now();
    copy("A1", "ZZ65536");
    ASSERT_TRUE(session.clipboard().has_value());
    EXPECT_EQ(session.clipboard()->items.size(), 2U);
    paste_at("A1");
    EXPECT_FALSE(failed()) << session.status().text;
    select("A1", "ZZ65536");
    press({"delete"});
    EXPECT_EQ(raw("ZZ65536"), "");
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(5));
}

TEST_F(SelectionTest, CommandsCanBeUsedDirectly) {
    input("A1", "1");
    input("B1", "=A1+1");
    Clipboard clipboard{.source = range("B1", "B1"),
                        .items = {{.address = at("B1"), .content = {.raw = "=A1+1", .style = {}}}}};
    session.apply_edit(std::make_unique<PasteCommand>(clipboard, at("C1"), functions));
    EXPECT_EQ(raw("C1"), "=B1+1");
    session.apply_edit(std::make_unique<ss::app::ClearRangeCommand>(range("A1", "C1")));
    EXPECT_FALSE(session.document().used_range().has_value());
    EXPECT_TRUE(session.undo());
    EXPECT_EQ(shown("C1"), "3");
}

// документ заменили, пока шло выделение: оно не должно указывать в старый лист
TEST_F(SelectionTest, NewDocumentRestartsSelection) {
    go("C3");
    press({"f3", "down"});
    session.replace_document(std::make_unique<ss::core::Spreadsheet>(functions, value_parsers), "other.sst");
    EXPECT_EQ(session.selection(), range("A1", "A1"));
}
