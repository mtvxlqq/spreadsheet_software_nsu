#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "app/cell_presenter.hpp"
#include "app/set_style_command.hpp"
#include "core/address.hpp"
#include "core/cell_style.hpp"
#include "core/errors.hpp"
#include "support/printers.hpp"
#include "support/session_fixture.hpp"

using ss::app::Align;
using ss::app::CellPresenter;
using ss::app::SetStyleCommand;
using ss::core::CellStyle;
using ss::core::Range;
using ss::test::at;

namespace {

class StyleTest : public ss::test::SessionTest {
protected:
    // текст ячейки так, как он попадёт на экран
    std::string text(std::string_view address) const {
        return CellPresenter::present(session.document(), at(address), number_formats).text;
    }

    const CellStyle& style(std::string_view address) const {
        return session.document().style(at(address));
    }
};

}  // namespace

TEST_F(StyleTest, FormatFixedRoundsOnScreenOnly) {
    input("A1", "=2/3");
    go("A1");
    command("format fixed 2");
    EXPECT_FALSE(failed());
    EXPECT_EQ(text("A1"), "0.67");
    EXPECT_EQ(style("A1").format, "fixed:2");
    // само значение и формула не поменялись
    EXPECT_EQ(raw("A1"), "=2/3");
    EXPECT_EQ(shown("A1"), "0.6666666667");
}

TEST_F(StyleTest, FormatDoesNotChangeDependentFormulas) {
    input("A1", "0.125");
    input("A2", "=A1*8");
    go("A1");
    command("format percent 0");
    EXPECT_EQ(text("A1"), "13%");
    EXPECT_EQ(shown("A2"), "1");
}

TEST_F(StyleTest, FormatPercent) {
    input("A1", "0.256");
    go("A1");
    command("format percent 1");
    EXPECT_EQ(text("A1"), "25.6%");
    EXPECT_EQ(style("A1").format, "percent:1");
}

TEST_F(StyleTest, FormatWithoutDigitsUsesTwo) {
    input("A1", "1.5");
    go("A1");
    command("format fixed");
    EXPECT_EQ(text("A1"), "1.50");
}

TEST_F(StyleTest, FormatGeneralRemovesFormat) {
    input("A1", "1.5");
    go("A1");
    command("format fixed 3");
    command("format general");
    EXPECT_EQ(text("A1"), "1.5");
    EXPECT_EQ(style("A1"), CellStyle{});
}

TEST_F(StyleTest, FormatDoesNotTouchTextAndErrors) {
    input("A1", "hello");
    input("A2", "=1/0");
    go("A1");
    command("format fixed 2");
    go("A2");
    command("format fixed 2");
    EXPECT_EQ(text("A1"), "hello");
    EXPECT_EQ(text("A2"), "#DIV/0!");
}

TEST_F(StyleTest, UnknownFormatNameIsReported) {
    input("A1", "1.5");
    go("A1");
    command("format money 2");
    EXPECT_TRUE(failed());
    EXPECT_NE(session.status().text.find("money"), std::string::npos);
    EXPECT_EQ(style("A1"), CellStyle{});
    // строка осталась, чтобы её можно было поправить
    EXPECT_EQ(session.mode_name(), "command");
    EXPECT_EQ(session.edit_buffer(), "format money 2");
}

TEST_F(StyleTest, BadDigitsAreReported) {
    input("A1", "1.5");
    go("A1");
    for (const char* line : {"format fixed abc", "format fixed -1", "format fixed 16",
                             "format fixed 1.5", "format fixed 2 3", "format", "format general 2"}) {
        command(line);
        EXPECT_TRUE(failed()) << line;
        EXPECT_EQ(style("A1"), CellStyle{}) << line;
    }
    // ничего из этого не попало в историю
    press({"esc", "ctrl+z"});
    EXPECT_EQ(raw("A1"), "");
}

TEST_F(StyleTest, AlignSetsAndAutoRemoves) {
    input("A1", "12");
    go("A1");
    command("align left");
    EXPECT_EQ(style("A1").align, Align::Left);
    command("align center");
    EXPECT_EQ(style("A1").align, Align::Center);
    command("align right");
    EXPECT_EQ(style("A1").align, Align::Right);
    command("align auto");
    EXPECT_EQ(style("A1"), CellStyle{});
}

TEST_F(StyleTest, BadAlignIsReported) {
    go("A1");
    for (const char* line : {"align", "align middle", "align left right"}) {
        command(line);
        EXPECT_TRUE(failed()) << line;
    }
    EXPECT_EQ(style("A1"), CellStyle{});
}

TEST_F(StyleTest, FormatAndAlignLiveTogether) {
    input("A1", "0.5");
    go("A1");
    command("format percent 0");
    command("align center");
    EXPECT_EQ(style("A1"), (CellStyle{.format = "percent:0", .align = Align::Center}));
    command("format general");
    EXPECT_EQ(style("A1"), (CellStyle{.format = "", .align = Align::Center}));
}

TEST_F(StyleTest, StyleIsUndoneAndRedone) {
    input("A1", "0.5");
    go("A1");
    command("format percent 0");
    command("align center");
    press({"ctrl+z"});
    EXPECT_EQ(style("A1"), (CellStyle{.format = "percent:0", .align = std::nullopt}));
    press({"ctrl+z"});
    EXPECT_EQ(style("A1"), CellStyle{});
    EXPECT_EQ(raw("A1"), "0.5");
    press({"ctrl+y", "ctrl+y"});
    EXPECT_EQ(style("A1"), (CellStyle{.format = "percent:0", .align = Align::Center}));
}

TEST_F(StyleTest, StyleMakesDocumentDirty) {
    session.mark_saved("table.sst");
    go("B2");
    command("align right");
    EXPECT_TRUE(session.dirty());
}

// стиль бывает и у пустой ячейки, и на used_range он не влияет
TEST_F(StyleTest, EmptyCellKeepsStyleForLaterInput) {
    go("C3");
    command("format fixed 1");
    EXPECT_FALSE(session.document().used_range().has_value());
    input("C3", "2.25");
    EXPECT_EQ(text("C3"), "2.3");
}

TEST_F(StyleTest, ClearingCellKeepsStyle) {
    input("A1", "2.25");
    go("A1");
    command("format fixed 1");
    press({"delete"});
    EXPECT_EQ(raw("A1"), "");
    EXPECT_EQ(style("A1").format, "fixed:1");
}

TEST_F(StyleTest, CommandChangesWholeRange) {
    auto command = std::make_unique<SetStyleCommand>(
        Range(at("A1"), at("B2")), [](CellStyle& style) { style.align = Align::Center; });
    session.apply_edit(std::move(command));
    for (const char* address : {"A1", "A2", "B1", "B2"}) {
        EXPECT_EQ(style(address).align, Align::Center) << address;
    }
    EXPECT_EQ(style("C1"), CellStyle{});
    EXPECT_TRUE(session.undo());
    EXPECT_EQ(style("B2"), CellStyle{});
}

TEST_F(StyleTest, HugeRangeIsRejected) {
    auto command = std::make_unique<SetStyleCommand>(
        Range(at("A1"), at("ZZ65536")), [](CellStyle& style) { style.align = Align::Center; });
    EXPECT_THROW(session.apply_edit(std::move(command)), ss::core::LimitError);
    EXPECT_EQ(style("A1"), CellStyle{});
    EXPECT_FALSE(session.undo());
}

// дата прижата вправо, как число, а формат чисел на неё не действует
TEST_F(StyleTest, DateIsShownRightAlignedAndIgnoresNumberFormat) {
    input("A1", "15.03.2024");
    const ss::app::CellView view =
        CellPresenter::present(session.document(), at("A1"), number_formats);
    EXPECT_EQ(view.text, "2024-03-15");
    EXPECT_EQ(view.align, Align::Right);
    EXPECT_TRUE(view.is_number);

    go("A1");
    command("format fixed 2");
    EXPECT_EQ(text("A1"), "2024-03-15");
    // а разность дат это обычное число, и формат на него действует
    input("A2", "01.03.2024");
    input("A3", "=A1-A2");
    go("A3");
    command("format fixed 1");
    EXPECT_EQ(text("A3"), "14.0");
}
