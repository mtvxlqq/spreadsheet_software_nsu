#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/value.hpp"
#include "support/manual_dispatcher.hpp"
#include "support/printers.hpp"

namespace fs = std::filesystem;

using ss::app::Session;
using ss::app::Status;
using ss::core::CellAddress;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

void write_file(const fs::path& path, std::string_view text) {
    std::ofstream out(path, std::ios::binary);
    out << text;
}

// по клавише на символ UTF-8, а не на байт
std::vector<std::string> keys_of(std::string_view text) {
    std::vector<std::string> keys;
    for (const char c : text) {
        const bool continuation = (static_cast<unsigned char>(c) >> 6) == 0x2;
        if (continuation && !keys.empty()) {
            keys.back() += c;
        } else {
            keys.emplace_back(1, c);
        }
    }
    return keys;
}

class ActionsTest : public ::testing::Test {
protected:
    ActionsTest() {
        ss::core::register_builtins(functions);
        ss::core::register_formats(formats);
        ss::app::register_actions(actions);
        session.resize(20, 8);
        // у каждого теста своя папка, тесты могут идти параллельно
        const std::string test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
        dir = fs::temp_directory_path() / ("ss_actions_" + test_name);
        fs::remove_all(dir);
        fs::create_directories(dir);
    }

    ~ActionsTest() override {
        std::error_code ignored;
        fs::remove_all(dir, ignored);
    }

    void type(std::string_view text) {
        for (const std::string& key : keys_of(text)) {
            session.handle_key(key);
        }
    }

    // как будто пользователь набрал ":line" и enter
    void command(std::string_view line) {
        // после неудачной команды строка остаётся открытой, закрываем её
        if (session.mode_name() == "command") {
            session.handle_key("esc");
        }
        session.handle_key(":");
        type(line);
        session.handle_key("enter");
        finish_background();
    }

    // open и import грузят в фоне, дожидаемся и выполняем продолжения
    void finish_background() {
        while (session.busy()) {
            ASSERT_TRUE(dispatcher.wait_for_task());
            dispatcher.drain();
        }
    }

    void input(std::string_view address, std::string_view raw) {
        session.set_cursor(at(address));
        // enter, а не первый символ: "k" из "keep" это move-up
        session.handle_key("enter");
        session.edit_line().clear();
        type(raw);
        session.handle_key("enter");
    }

    std::string shown(std::string_view address) const {
        return session.document().value(at(address))->to_text();
    }

    std::string file(std::string_view name) const {
        return (dir / name).string();
    }

    bool failed() const {
        return session.status().kind == Status::Kind::Error;
    }

    ss::core::FunctionRegistry functions;
    ss::core::FormatRegistry formats;
    ss::app::ActionRegistry actions;
    ss::test::ManualDispatcher dispatcher;
    Session session{functions, formats, actions, ss::app::Keymap::defaults(), dispatcher};
    fs::path dir;
};

}  // namespace

TEST_F(ActionsTest, NavigationByName) {
    command("move-down");
    command("move-right");
    EXPECT_EQ(session.cursor(), at("B2"));
    command("move-up");
    command("move-left");
    EXPECT_EQ(session.cursor(), at("A1"));
    command("page-down");
    EXPECT_EQ(session.cursor(), at("A21"));
    command("page-up");
    EXPECT_EQ(session.cursor(), at("A1"));
    session.set_cursor(at("D7"));
    command("go-home");
    EXPECT_EQ(session.cursor(), at("A7"));
    command("go-top");
    EXPECT_EQ(session.cursor(), at("A1"));
}

TEST_F(ActionsTest, EditingByName) {
    input("A1", "=2+2");
    session.set_cursor(at("A1"));
    command("start-edit");
    EXPECT_EQ(session.mode_name(), "edit");
    EXPECT_EQ(session.edit_buffer(), "=2+2");
    session.handle_key("esc");
    command("clear-cell");
    EXPECT_EQ(session.document().raw(at("A1")), "");
}

TEST_F(ActionsTest, ModesByName) {
    command("command-mode");
    EXPECT_EQ(session.mode_name(), "command");
    session.handle_key("esc");
    command("show-help");
    EXPECT_EQ(session.mode_name(), "help");
    session.handle_key("x");
    command("help");
    EXPECT_EQ(session.mode_name(), "help");
}

TEST_F(ActionsTest, SaveWritesFile) {
    input("A1", "42");
    input("B2", "=A1*2");
    command("w " + file("t.sst"));
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(read_file(dir / "t.sst"), "SST 1\nA1\t42\nB2\t=A1*2\n");
    EXPECT_FALSE(session.dirty());
    EXPECT_EQ(session.current_path(), fs::path(file("t.sst")));
    // временный файл не остался
    EXPECT_FALSE(fs::exists(dir / "t.sst.tmp"));
}

TEST_F(ActionsTest, SaveWithoutPathUsesCurrentFile) {
    input("A1", "1");
    command("save " + file("t.sst"));
    input("A2", "2");
    EXPECT_TRUE(session.dirty());
    command("w");
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(read_file(dir / "t.sst"), "SST 1\nA1\t1\nA2\t2\n");
    EXPECT_FALSE(session.dirty());
}

TEST_F(ActionsTest, CtrlSSavesToCurrentFile) {
    command("w " + file("t.sst"));
    input("A1", "5");
    session.handle_key("ctrl+s");
    EXPECT_EQ(read_file(dir / "t.sst"), "SST 1\nA1\t5\n");
}

TEST_F(ActionsTest, SaveWithoutAnyPathFails) {
    input("A1", "1");
    command("w");
    EXPECT_TRUE(failed());
    EXPECT_TRUE(session.dirty());
}

TEST_F(ActionsTest, SaveToCsvIsNotSupported) {
    input("A1", "1");
    command("w " + file("t.csv"));
    EXPECT_TRUE(failed());
    EXPECT_FALSE(fs::exists(dir / "t.csv"));
    EXPECT_TRUE(session.dirty());
}

TEST_F(ActionsTest, SaveIntoMissingDirectoryFails) {
    command("w " + file("no/such/dir/t.sst"));
    EXPECT_TRUE(failed());
}

TEST_F(ActionsTest, OpenReplacesDocument) {
    write_file(dir / "t.sst", "SST 1\nA1\t10\nA2\t=A1+5\n");
    input("C3", "old");
    session.set_cursor(at("C3"));
    command("e " + file("t.sst"));
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(shown("A2"), "15");
    EXPECT_EQ(session.document().raw(at("C3")), "");
    EXPECT_EQ(session.cursor(), at("A1"));
    EXPECT_FALSE(session.dirty());
    EXPECT_EQ(session.current_path(), fs::path(file("t.sst")));
}

TEST_F(ActionsTest, SaveAndOpenRoundTrip) {
    input("A1", "привет");
    input("A2", "=A3*2");
    input("A3", "3");
    command("w " + file("t.sst"));
    command("e " + file("t.sst"));
    EXPECT_EQ(shown("A1"), "привет");
    EXPECT_EQ(shown("A2"), "6");
    EXPECT_EQ(session.document().raw(at("A2")), "=A3*2");
}

TEST_F(ActionsTest, OpenMissingFileKeepsDocument) {
    input("A1", "keep");
    command("open " + file("nope.sst"));
    EXPECT_TRUE(failed());
    EXPECT_EQ(shown("A1"), "keep");
    EXPECT_TRUE(session.dirty());
}

TEST_F(ActionsTest, OpenBrokenFileKeepsDocument) {
    write_file(dir / "bad.sst", "SST 1\nA1\t1\nB1\t=1+\n");
    input("A1", "keep");
    command("e " + file("bad.sst"));
    EXPECT_TRUE(failed());
    EXPECT_NE(session.status().text.find("строка 3"), std::string::npos)
        << session.status().text;
    EXPECT_EQ(shown("A1"), "keep");
    EXPECT_EQ(session.current_path(), std::nullopt);
}

TEST_F(ActionsTest, OpenUnknownFormatFails) {
    command("e " + file("t.xlsx"));
    EXPECT_TRUE(failed());
}

TEST_F(ActionsTest, OpenNeedsExactlyOnePath) {
    command("e");
    EXPECT_TRUE(failed());
    command("e a.sst b.sst");
    EXPECT_TRUE(failed());
}

TEST_F(ActionsTest, ImportCsv) {
    write_file(dir / "data.csv", "name,value\nx,1\ny,=1+1\n");
    command("import " + file("data.csv"));
    EXPECT_FALSE(failed()) << session.status().text;
    EXPECT_EQ(shown("A1"), "name");
    EXPECT_EQ(shown("B2"), "1");
    EXPECT_EQ(shown("B3"), "=1+1");
    // документ новый и ещё не сохранён
    EXPECT_TRUE(session.dirty());
    EXPECT_EQ(session.current_path(), std::nullopt);
    command("w");
    EXPECT_TRUE(failed());
    command("w " + file("data.sst"));
    EXPECT_FALSE(failed()) << session.status().text;
}

TEST_F(ActionsTest, BrokenCsvKeepsDocument) {
    write_file(dir / "bad.csv", "a,\"b\n");
    input("A1", "keep");
    command("import-csv " + file("bad.csv"));
    EXPECT_TRUE(failed());
    EXPECT_EQ(shown("A1"), "keep");
}

TEST_F(ActionsTest, QuitRefusesWithUnsavedChanges) {
    input("A1", "1");
    command("q");
    EXPECT_TRUE(failed());
    EXPECT_NE(session.status().text.find(":q!"), std::string::npos);
    EXPECT_FALSE(session.quit_requested());
    command("q!");
    EXPECT_TRUE(session.quit_requested());
}

TEST_F(ActionsTest, QuitWhenSaved) {
    input("A1", "1");
    command("w " + file("t.sst"));
    session.handle_key("ctrl+q");
    EXPECT_TRUE(session.quit_requested());
}

TEST_F(ActionsTest, QuitOnEmptyDocument) {
    command("quit");
    EXPECT_TRUE(session.quit_requested());
}

TEST_F(ActionsTest, UnknownCommandKeepsProgramAlive) {
    command("xyz");
    EXPECT_TRUE(failed());
    session.handle_key("esc");
    input("A1", "1");
    EXPECT_EQ(shown("A1"), "1");
}

TEST_F(ActionsTest, HelpDescribesEveryAction) {
    const std::vector<std::string> lines = session.help_lines();
    for (const std::string& name : actions.names()) {
        const std::string description(actions.create(name)->description());
        const bool found = std::ranges::any_of(lines, [&](const std::string& line) {
            return line.starts_with(name) && line.ends_with(description);
        });
        EXPECT_TRUE(found) << name;
    }
    EXPECT_NE(std::ranges::find(lines, "save [ctrl+s]: сохранить, можно указать файл (:w путь)"),
              lines.end());
}

TEST_F(ActionsTest, DefaultKeysUseRegisteredActions) {
    const std::string config = std::string(SS_TESTDATA_DIR) + "/../config/keys.conf";
    EXPECT_TRUE(ss::app::Keymap::load(config, actions.names()).warnings.empty());
    for (const std::string& name : actions.names()) {
        EXPECT_NO_THROW(static_cast<void>(actions.create(name))) << name;
    }
}
