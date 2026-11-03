#include <gtest/gtest.h>

#include "app/line_editor.hpp"

using ss::app::LineEditor;

TEST(LineEditorTest, InsertsAtCaret) {
    LineEditor line;
    line.insert("ac");
    line.move_left();
    line.insert("b");
    EXPECT_EQ(line.text(), "abc");
    EXPECT_EQ(line.caret(), 2U);
}

TEST(LineEditorTest, SetTextPutsCaretAtEnd) {
    LineEditor line;
    line.set_text("=A1+1");
    EXPECT_EQ(line.caret(), 5U);
    line.clear();
    EXPECT_EQ(line.text(), "");
    EXPECT_EQ(line.caret(), 0U);
}

TEST(LineEditorTest, BackspaceAndDelete) {
    LineEditor line;
    line.set_text("abcd");
    line.backspace();
    EXPECT_EQ(line.text(), "abc");
    line.move_home();
    line.backspace();  // в начале ничего не делает
    line.erase_forward();
    EXPECT_EQ(line.text(), "bc");
    line.move_end();
    line.erase_forward();  // в конце тоже
    EXPECT_EQ(line.text(), "bc");
}

TEST(LineEditorTest, CaretStopsAtEdges) {
    LineEditor line;
    line.set_text("ab");
    line.move_right();
    EXPECT_EQ(line.caret(), 2U);
    line.move_left();
    line.move_left();
    line.move_left();
    EXPECT_EQ(line.caret(), 0U);
}

TEST(LineEditorTest, WorksWithWholeUtf8Characters) {
    LineEditor line;
    line.set_text("привет");
    line.backspace();
    EXPECT_EQ(line.text(), "приве");
    line.move_left();
    EXPECT_EQ(line.caret(), 8U);  // каждая буква по 2 байта
    line.erase_forward();
    EXPECT_EQ(line.text(), "прив");
    line.move_home();
    line.move_right();
    line.insert("ё");
    EXPECT_EQ(line.text(), "пёрив");
}

TEST(LineEditorTest, HandlesOnlyEditingKeys) {
    LineEditor line;
    EXPECT_TRUE(line.handle_key("a"));
    EXPECT_TRUE(line.handle_key("ж"));
    EXPECT_TRUE(line.handle_key(" "));
    EXPECT_TRUE(line.handle_key("left"));
    EXPECT_TRUE(line.handle_key("backspace"));
    EXPECT_EQ(line.text(), "a ");
    EXPECT_FALSE(line.handle_key("enter"));
    EXPECT_FALSE(line.handle_key("up"));
    EXPECT_FALSE(line.handle_key("ctrl+s"));
    EXPECT_EQ(line.text(), "a ");
}
