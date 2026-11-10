#include <cstddef>
#include <string>
#include <string_view>

#include <ftxui/screen/string.hpp>
#include <gtest/gtest.h>

#include "app/cell_presenter.hpp"
#include "tui/text_fit.hpp"

using ss::app::Align;
using ss::tui::caret_window;
using ss::tui::CaretWindow;
using ss::tui::fit_cell;
using ss::tui::Overflow;

namespace {

// символ под кареткой в квадратных скобках, так удобнее сравнивать
std::string shown(const CaretWindow& window) {
    return window.before + "[" + window.at + "]" + window.after;
}

}  // namespace

TEST(FitCellTest, ShortTextIsPadded) {
    EXPECT_EQ(fit_cell("abc", 6, Align::Left), "abc   ");
    EXPECT_EQ(fit_cell("12", 6, Align::Right), "    12");
}

TEST(FitCellTest, ExactFitIsUnchanged) {
    EXPECT_EQ(fit_cell("abcd", 4, Align::Left), "abcd");
    EXPECT_EQ(fit_cell("1234", 4, Align::Right), "1234");
}

TEST(FitCellTest, EmptyTextIsAllSpaces) {
    EXPECT_EQ(fit_cell("", 3, Align::Left), "   ");
    EXPECT_EQ(fit_cell("", 3, Align::Right), "   ");
}

TEST(FitCellTest, LongTextIsCut) {
    EXPECT_EQ(fit_cell("abcdefgh", 4, Align::Left), "abcd");
}

TEST(FitCellTest, LongNumberBecomesHashes) {
    EXPECT_EQ(fit_cell("123456", 4, Align::Right, Overflow::Hashes), "####");
    // решётки зависят от того, число ли это, а не от выравнивания
    EXPECT_EQ(fit_cell("123456", 4, Align::Left, Overflow::Hashes), "####");
    EXPECT_EQ(fit_cell("123456", 4, Align::Center, Overflow::Hashes), "####");
    EXPECT_EQ(fit_cell("abcdef", 4, Align::Right), "abcd");
    // а что влезло, остаётся как есть
    EXPECT_EQ(fit_cell("1234", 4, Align::Left, Overflow::Hashes), "1234");
}

TEST(FitCellTest, CenterSplitsPaddingEvenly) {
    EXPECT_EQ(fit_cell("ab", 6, Align::Center), "  ab  ");
    // лишний пробел уходит вправо
    EXPECT_EQ(fit_cell("abc", 6, Align::Center), " abc  ");
    EXPECT_EQ(fit_cell("abcdef", 6, Align::Center), "abcdef");
    EXPECT_EQ(fit_cell("abcdefgh", 6, Align::Center), "abcdef");
    EXPECT_EQ(fit_cell("", 3, Align::Center), "   ");
}

// ширина считается в позициях экрана, а не в байтах
TEST(FitCellTest, CenterCountsCyrillicByCharacters) {
    EXPECT_EQ(fit_cell("да", 6, Align::Center), "  да  ");
    EXPECT_EQ(fit_cell("кот", 8, Align::Center), "  кот   ");
    EXPECT_EQ(fit_cell("日本", 6, Align::Center), " 日本 ");
}

TEST(FitCellTest, CyrillicIsNotBroken) {
    EXPECT_EQ(fit_cell("привет", 4, Align::Left), "прив");
    EXPECT_EQ(fit_cell("да", 4, Align::Left), "да  ");
}

// половина иероглифа не влезает, остаётся пробел
TEST(FitCellTest, WideCharacterDoesNotOverflow) {
    const std::string fitted = fit_cell("日本語", 5, Align::Left);
    EXPECT_EQ(fitted, "日本 ");
    EXPECT_EQ(ftxui::string_width(fitted), 5);
}

TEST(FitCellTest, CombiningMarkTakesNoSpace) {
    EXPECT_EQ(fit_cell("e\xCC\x81x", 2, Align::Left), "e\xCC\x81x");
}

// многострочная ячейка из CSV не должна слипаться
TEST(FitCellTest, ControlCharactersBecomeSpaces) {
    EXPECT_EQ(fit_cell("a\nb", 5, Align::Left), "a b  ");
    EXPECT_EQ(fit_cell("a\tb", 3, Align::Left), "a b");
}

TEST(FitCellTest, ZeroOrNegativeWidthIsEmpty) {
    EXPECT_EQ(fit_cell("abc", 0, Align::Left), "");
    EXPECT_EQ(fit_cell("abc", -1, Align::Right), "");
}

TEST(FitCellTest, BrokenUtf8KeepsWidth) {
    EXPECT_EQ(ftxui::string_width(fit_cell("\xFF\xFE", 4, Align::Left)), 4);
}

TEST(CaretWindowTest, CaretAtStart) {
    EXPECT_EQ(shown(caret_window("abc", 0, 10)), "[a]bc");
}

TEST(CaretWindowTest, CaretInMiddle) {
    EXPECT_EQ(shown(caret_window("abc", 1, 10)), "a[b]c");
}

TEST(CaretWindowTest, CaretAtEndIsSpace) {
    EXPECT_EQ(shown(caret_window("abc", 3, 10)), "abc[ ]");
}

TEST(CaretWindowTest, EmptyText) {
    EXPECT_EQ(shown(caret_window("", 0, 10)), "[ ]");
}

TEST(CaretWindowTest, CaretPastEndIsTreatedAsEnd) {
    EXPECT_EQ(shown(caret_window("abc", 100, 10)), "abc[ ]");
}

TEST(CaretWindowTest, LongTextCaretAtStartShowsBeginning) {
    EXPECT_EQ(shown(caret_window("abcdefghij", 0, 4)), "[a]bcd");
}

TEST(CaretWindowTest, LongTextCaretAtEndShowsTail) {
    EXPECT_EQ(shown(caret_window("abcdefghij", 10, 4)), "hij[ ]");
}

TEST(CaretWindowTest, LongTextCaretOutsideShiftsWindow) {
    EXPECT_EQ(shown(caret_window("abcdefghij", 8, 4)), "fgh[i]");
}

// каретка считается в байтах, а 'ж' занимает два
TEST(CaretWindowTest, CyrillicCaretInBytes) {
    EXPECT_EQ(shown(caret_window("жук", 2, 10)), "ж[у]к");
}

TEST(CaretWindowTest, WideCharacters) {
    EXPECT_EQ(shown(caret_window("日本語", 6, 4)), "本[語]");
}

TEST(CaretWindowTest, WideCharacterInNarrowWindow) {
    EXPECT_EQ(shown(caret_window("日", 0, 1)), "[ ]");
}

TEST(CaretWindowTest, ControlCharactersKeepOffsets) {
    EXPECT_EQ(shown(caret_window("a\nb", 2, 10)), "a [b]");
}

TEST(CaretWindowTest, ZeroWidthIsEmpty) {
    EXPECT_EQ(shown(caret_window("abc", 1, 0)), "[]");
}

TEST(CaretWindowTest, NeverWiderThanWindow) {
    const std::string_view text = "aжb日c語d";
    for (int width = 1; width <= 12; ++width) {
        for (std::size_t caret = 0; caret <= text.size(); ++caret) {
            const CaretWindow window = caret_window(text, caret, width);
            EXPECT_LE(ftxui::string_width(window.before + window.at + window.after), width);
            EXPECT_FALSE(window.at.empty());
        }
    }
}
