#include <functional>
#include <initializer_list>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "core/builtins.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format_registry.hpp"
#include "support/manual_dispatcher.hpp"
#include "tui/screen_layout.hpp"
#include "tui/screen_view.hpp"

using ftxui::Screen;
using ss::app::Session;
using ss::tui::compute_layout;
using ss::tui::render_screen;

namespace {

constexpr int first_grid_line = 2;
constexpr int first_cell_x = ss::tui::row_header_width;

std::string line_of(const Screen& screen, int y) {
    std::string line;
    for (int x = 0; x < screen.dimx(); ++x) {
        line += screen.PixelAt(x, y).character;
    }
    return line;
}

bool contains(const std::string& text, std::string_view part) {
    return text.find(part) != std::string::npos;
}

class ScreenViewTest : public ::testing::Test {
protected:
    ScreenViewTest() {
        ss::core::register_builtins(functions);
        ss::core::register_formats(formats);
        ss::core::register_number_formats(number_formats);
        ss::app::register_actions(actions);
        use_window(80, 24);
    }

    void use_window(int width, int height) {
        const ss::tui::ScreenLayout layout = compute_layout(width, height);
        session.resize(layout.visible_rows, layout.visible_cols);
    }

    Screen draw(int width = 80, int height = 24) const {
        Screen screen =
            Screen::Create(ftxui::Dimension::Fixed(width), ftxui::Dimension::Fixed(height));
        ftxui::Render(screen, render_screen(session.view(), width));
        return screen;
    }

    void press(std::initializer_list<std::string_view> keys) {
        for (const std::string_view key : keys) {
            session.handle_key(key);
        }
    }

    void press_times(std::string_view key, int times) {
        for (int i = 0; i < times; ++i) {
            session.handle_key(key);
        }
    }

    // записать в текущую ячейку, курсор уйдёт на строку ниже. Только латиница.
    void fill(std::string_view text) {
        session.handle_key("enter");
        for (const char c : text) {
            session.handle_key(std::string(1, c));
        }
        session.handle_key("enter");
    }

    ss::core::FunctionRegistry functions;
    ss::core::FormatRegistry formats;
    ss::core::NumberFormatRegistry number_formats;
    ss::app::ActionRegistry actions;
    ss::test::ManualDispatcher dispatcher;
    Session session{functions, formats, number_formats, actions,
                    ss::app::Keymap::defaults(), dispatcher};
};

}  // namespace

TEST_F(ScreenViewTest, ColumnAndRowHeaders) {
    const Screen screen = draw();
    const std::string header = line_of(screen, 1);
    EXPECT_EQ(header.substr(first_cell_x, 1), "A");
    EXPECT_EQ(header.substr(first_cell_x + 10, 1), "B");
    EXPECT_EQ(header.substr(first_cell_x + 60, 1), "G");
    EXPECT_EQ(line_of(screen, first_grid_line).substr(0, 5), "    1");
    EXPECT_EQ(line_of(screen, first_grid_line + 20).substr(0, 5), "   21");
}

TEST_F(ScreenViewTest, NumbersGoRightAndTextGoesLeft) {
    fill("12");
    fill("abc");
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, first_grid_line).substr(first_cell_x, 9), "       12");
    EXPECT_EQ(line_of(screen, first_grid_line + 1).substr(first_cell_x, 9), "abc      ");
}

TEST_F(ScreenViewTest, ErrorValueIsShown) {
    fill("=1/0");
    EXPECT_TRUE(contains(line_of(draw(), first_grid_line), "#DIV/0!"));
}

TEST_F(ScreenViewTest, FormulaShowsResult) {
    fill("=1+2");
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 9), "        3");
}

TEST_F(ScreenViewTest, LongNumberBecomesHashes) {
    fill("123456789012");
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 9), "#########");
}

TEST_F(ScreenViewTest, NumberFormatOfCellIsShown) {
    fill("0.256");
    press({"up", ":"});
    for (const char c : std::string_view("format percent 1")) {
        session.handle_key(std::string(1, c));
    }
    press({"enter"});
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 9), "    25.6%");
    // в строке формулы по-прежнему то, что ввёл пользователь
    EXPECT_EQ(line_of(draw(), 0).substr(0, 9), "A1: 0.256");

    press({"ctrl+z"});
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 9), "    0.256");
}

TEST_F(ScreenViewTest, AlignOfCellIsShown) {
    fill("12");
    fill("abc");
    press({"up", "up", ":"});
    for (const char c : std::string_view("align center")) {
        session.handle_key(std::string(1, c));
    }
    press({"enter", "down", ":"});
    for (const char c : std::string_view("align right")) {
        session.handle_key(std::string(1, c));
    }
    press({"enter"});
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, first_grid_line).substr(first_cell_x, 9), "   12    ");
    EXPECT_EQ(line_of(screen, first_grid_line + 1).substr(first_cell_x, 9), "      abc");
}

// число, которое не влезло, даёт решётки при любом выравнивании
TEST_F(ScreenViewTest, LongNumberBecomesHashesWhenAlignedLeft) {
    fill("123456789012");
    press({"up", ":"});
    for (const char c : std::string_view("align left")) {
        session.handle_key(std::string(1, c));
    }
    press({"enter"});
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 9), "#########");
}

TEST_F(ScreenViewTest, LongTextIsCutAtColumn) {
    fill("abcdefghijklmnop");
    EXPECT_EQ(line_of(draw(), first_grid_line).substr(first_cell_x, 10), "abcdefghi ");
}

TEST_F(ScreenViewTest, CyrillicCell) {
    press({"enter", "ж", "у", "к", "enter"});
    EXPECT_TRUE(contains(line_of(draw(), first_grid_line), "жук"));
}

TEST_F(ScreenViewTest, CursorCellIsInverted) {
    Screen screen = draw();
    EXPECT_TRUE(screen.PixelAt(first_cell_x, first_grid_line).inverted);
    EXPECT_TRUE(screen.PixelAt(first_cell_x + 9, first_grid_line).inverted);
    EXPECT_FALSE(screen.PixelAt(first_cell_x + 10, first_grid_line).inverted);

    press({"right"});
    screen = draw();
    EXPECT_FALSE(screen.PixelAt(first_cell_x, first_grid_line).inverted);
    EXPECT_TRUE(screen.PixelAt(first_cell_x + 10, first_grid_line).inverted);
}

TEST_F(ScreenViewTest, ScrollsToTheRight) {
    press_times("right", 7);
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, 1).substr(first_cell_x, 1), "B");
    EXPECT_TRUE(screen.PixelAt(first_cell_x + 60, first_grid_line).inverted);
}

TEST_F(ScreenViewTest, ScrollsDown) {
    press_times("down", 21);
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, first_grid_line).substr(0, 5), "    2");
    EXPECT_TRUE(screen.PixelAt(first_cell_x, first_grid_line + 20).inverted);
}

TEST_F(ScreenViewTest, FormulaBarShowsRawText) {
    fill("=1+2");
    press({"up"});
    EXPECT_EQ(line_of(draw(), 0).substr(0, 8), "A1: =1+2");
}

TEST_F(ScreenViewTest, EditShowsCaretAfterText) {
    fill("=1+2");
    press({"up", "enter"});
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, 0).substr(0, 8), "A1: =1+2");
    EXPECT_TRUE(screen.PixelAt(8, 0).inverted);
    EXPECT_FALSE(screen.PixelAt(7, 0).inverted);
}

TEST_F(ScreenViewTest, LongEditKeepsCaretVisible) {
    press({"enter"});
    press_times("1", 200);
    EXPECT_TRUE(draw().PixelAt(79, 0).inverted);
}

TEST_F(ScreenViewTest, CommandLineReplacesFormulaBar) {
    press({":", "w"});
    const Screen screen = draw();
    EXPECT_EQ(line_of(screen, 0).substr(0, 2), ":w");
    EXPECT_TRUE(screen.PixelAt(2, 0).inverted);
    EXPECT_TRUE(contains(line_of(screen, 23), "command"));
}

TEST_F(ScreenViewTest, StatusShowsModeAndUnnamedDocument) {
    const std::string status = line_of(draw(), 23);
    EXPECT_TRUE(contains(status, "normal"));
    EXPECT_TRUE(contains(status, "без имени"));
    EXPECT_FALSE(contains(status, "*"));
}

TEST_F(ScreenViewTest, StatusMarksUnsavedChanges) {
    fill("1");
    EXPECT_TRUE(contains(line_of(draw(), 23), "без имени*"));
}

TEST_F(ScreenViewTest, StatusShowsFileNameWithoutFolders) {
    fill("1");
    session.mark_saved("/some/folder/table.sst");
    const std::string status = line_of(draw(), 23);
    EXPECT_TRUE(contains(status, "table.sst"));
    EXPECT_FALSE(contains(status, "folder"));
    EXPECT_FALSE(contains(status, "*"));
}

TEST_F(ScreenViewTest, ErrorMessageIsRed) {
    press({"enter", "=", "1", "+", "enter"});
    EXPECT_EQ(session.status().kind, ss::app::Status::Kind::Error);
    const Screen screen = draw();
    EXPECT_TRUE(contains(line_of(screen, 23), "ошибка в формуле"));
    bool has_red = false;
    for (int x = 0; x < screen.dimx(); ++x) {
        has_red = has_red || screen.PixelAt(x, 23).foreground_color == ftxui::Color::Red;
    }
    EXPECT_TRUE(has_red);
}

// длинное сообщение обрезается само и не сжимает режим и имя файла
TEST_F(ScreenViewTest, LongMessageKeepsModeAndName) {
    session.set_status(std::string(300, 'x'), ss::app::Status::Kind::Error);
    const std::string status = line_of(draw(), 23);
    EXPECT_EQ(status.substr(0, 8), "normal  ");
    EXPECT_TRUE(contains(status, "без имени"));
    EXPECT_EQ(status.substr(status.size() - 1), "x");
}

TEST_F(ScreenViewTest, InfoMessageIsNotRed) {
    session.set_status("всё хорошо");
    const Screen screen = draw();
    EXPECT_TRUE(contains(line_of(screen, 23), "всё хорошо"));
    for (int x = 0; x < screen.dimx(); ++x) {
        EXPECT_FALSE(screen.PixelAt(x, 23).foreground_color == ftxui::Color::Red);
    }
}

TEST_F(ScreenViewTest, BusyIsShownWhenStatusIsEmpty) {
    session.run_in_background(
        [](std::stop_token) { return std::function<void(Session&)>([](Session&) {}); });
    ASSERT_TRUE(session.busy());
    EXPECT_TRUE(contains(line_of(draw(), 23), "загрузка"));
}

TEST_F(ScreenViewTest, HelpReplacesTableAndFitsStandardWindow) {
    press({"?"});
    const Screen screen = draw();
    const std::vector<std::string> help = session.help_lines();
    ASSERT_LE(help.size() + 2, 24U);
    for (std::size_t i = 0; i < help.size(); ++i) {
        const std::string name = help[i].substr(0, help[i].find_first_of(" :"));
        EXPECT_TRUE(contains(line_of(screen, static_cast<int>(i) + 1), name)) << name;
    }
    EXPECT_FALSE(contains(line_of(screen, 1), "A         B"));
    EXPECT_TRUE(contains(line_of(screen, 23), "help"));
}

TEST_F(ScreenViewTest, TinyWindowDoesNotCrash) {
    use_window(1, 1);
    EXPECT_NO_THROW(draw(1, 1));
}

TEST_F(ScreenViewTest, NarrowEditDoesNotCrash) {
    use_window(3, 4);
    press({"enter", "1", "2"});
    EXPECT_NO_THROW(draw(3, 4));
}

TEST_F(ScreenViewTest, WindowWiderThanSheetDoesNotCrash) {
    session.resize(5, ss::core::max_cols + 100);
    EXPECT_NO_THROW(draw(8000, 8));
}
