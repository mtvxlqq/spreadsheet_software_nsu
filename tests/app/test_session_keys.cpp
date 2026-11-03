#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/mode.hpp"
#include "app/modes/help_mode.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "app/session_observer.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/value.hpp"
#include "support/printers.hpp"

using ss::app::Session;
using ss::app::Status;
using ss::core::CellAddress;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class CountingObserver : public ss::app::SessionObserver {
public:
    void on_session_changed() override {
        ++calls;
    }

    int calls = 0;
};

// отписывается прямо внутри уведомления
class LeavingObserver : public ss::app::SessionObserver {
public:
    explicit LeavingObserver(Session& session) : session_(session) {}

    void on_session_changed() override {
        ++calls;
        session_.unsubscribe(this);
    }

    int calls = 0;

private:
    Session& session_;
};

// После set_mode читает своё поле. Если бы Session удалила режим сразу
// в set_mode, это было бы чтение освобождённой памяти, и ASan бы упал.
class SelfReplacingMode final : public ss::app::Mode {
public:
    SelfReplacingMode(std::string name, std::string& last_seen)
        : name_(std::move(name)), last_seen_(last_seen) {}

    std::string_view name() const override {
        return name_;
    }

    void handle_key(Session& session, std::string_view /*key*/) const override {
        session.set_mode(std::make_unique<SelfReplacingMode>(name_ + "+", last_seen_));
        last_seen_ = name_;
    }

private:
    std::string name_;
    std::string& last_seen_;
};

class SessionKeysTest : public ::testing::Test {
protected:
    SessionKeysTest() {
        ss::core::register_builtins(functions);
        ss::core::register_formats(formats);
        ss::app::register_actions(actions);
        session.resize(20, 8);
    }

    void press(std::initializer_list<std::string_view> keys) {
        for (const std::string_view key : keys) {
            session.handle_key(key);
        }
    }

    // по клавише на каждый байт, так что только для латиницы
    void type(std::string_view text) {
        for (const char c : text) {
            session.handle_key(std::string(1, c));
        }
    }

    std::string shown(std::string_view address) const {
        return session.document().value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    ss::core::FormatRegistry formats;
    ss::app::ActionRegistry actions;
    Session session{functions, formats, actions, ss::app::Keymap::defaults()};
};

}  // namespace

TEST_F(SessionKeysTest, TypingStartsEditAndEnterCommits) {
    press({"1", "2", "enter"});
    EXPECT_EQ(session.document().raw(at("A1")), "12");
    EXPECT_EQ(shown("A1"), "12");
    EXPECT_EQ(session.cursor(), at("A2"));
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_TRUE(session.dirty());
}

TEST_F(SessionKeysTest, EscCancelsEdit) {
    press({"enter", "=", "1", "+", "esc"});
    EXPECT_EQ(session.document().raw(at("A1")), "");
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_EQ(session.cursor(), at("A1"));
    EXPECT_FALSE(session.dirty());
}

TEST_F(SessionKeysTest, BrokenFormulaKeepsEditAndText) {
    press({"=", "1", "+", "enter"});
    EXPECT_EQ(session.mode_name(), "edit");
    EXPECT_EQ(session.status().kind, Status::Kind::Error);
    EXPECT_FALSE(session.status().text.empty());
    EXPECT_EQ(session.edit_buffer(), "=1+");
    EXPECT_EQ(session.cursor(), at("A1"));
    EXPECT_EQ(session.document().raw(at("A1")), "");

    // дописали и всё получилось
    press({"2", "enter"});
    EXPECT_EQ(shown("A1"), "3");
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_EQ(session.status().kind, Status::Kind::Info);
}

TEST_F(SessionKeysTest, StartEditBeginsWithOldContent) {
    type("=1+2");
    press({"enter", "up", "f2"});
    EXPECT_EQ(session.mode_name(), "edit");
    EXPECT_EQ(session.edit_buffer(), "=1+2");
    press({"backspace", "5", "enter"});
    EXPECT_EQ(shown("A1"), "6");
}

TEST_F(SessionKeysTest, TypingReplacesOldContent) {
    type("old");
    press({"enter", "up"});
    type("x");
    EXPECT_EQ(session.edit_buffer(), "x");
}

TEST_F(SessionKeysTest, EditingInsideTheLine) {
    press({"a", "b", "left", "x", "end", "c", "enter"});
    EXPECT_EQ(shown("A1"), "axbc");
}

TEST_F(SessionKeysTest, RussianTextInCell) {
    press({"п", "р", "и", "backspace", "enter"});
    EXPECT_EQ(shown("A1"), "пр");
}

TEST_F(SessionKeysTest, TabCommitsAndGoesRight) {
    press({"7", "tab"});
    EXPECT_EQ(shown("A1"), "7");
    EXPECT_EQ(session.cursor(), at("B1"));
}

TEST_F(SessionKeysTest, FormulaBarShowsRawOrBuffer) {
    type("=2*3");
    press({"enter", "up"});
    EXPECT_EQ(session.formula_bar_text(), "=2*3");
    press({"9"});
    EXPECT_EQ(session.formula_bar_text(), "9");
}

TEST_F(SessionKeysTest, CursorStaysInsideSheet) {
    press({"up", "left", "k", "h"});
    EXPECT_EQ(session.cursor(), at("A1"));
    press({"pageup", "home", "ctrl+home"});
    EXPECT_EQ(session.cursor(), at("A1"));

    session.set_cursor({.row = ss::core::max_rows - 1, .col = ss::core::max_cols - 1});
    press({"down", "right", "j", "l", "pagedown"});
    EXPECT_EQ(session.cursor(), at("ZZ65536"));
}

TEST_F(SessionKeysTest, MovementKeys) {
    press({"down", "j", "right", "l"});
    EXPECT_EQ(session.cursor(), at("C3"));
    press({"pagedown"});
    EXPECT_EQ(session.cursor(), at("C23"));
    press({"home"});
    EXPECT_EQ(session.cursor(), at("A23"));
    press({"ctrl+home"});
    EXPECT_EQ(session.cursor(), at("A1"));
}

TEST_F(SessionKeysTest, ViewportFollowsCursor) {
    session.resize(5, 3);
    press({"down", "down", "down", "down", "down"});
    EXPECT_EQ(session.cursor(), at("A6"));
    EXPECT_EQ(session.viewport().first_row(), 1);
    press({"right", "right", "right"});
    EXPECT_EQ(session.viewport().first_col(), 1);
    press({"ctrl+home"});
    EXPECT_EQ(session.viewport().first_row(), 0);
    EXPECT_EQ(session.viewport().first_col(), 0);
}

TEST_F(SessionKeysTest, ResizeKeepsCursorVisible) {
    session.set_cursor(at("A15"));
    session.resize(5, 3);
    EXPECT_TRUE(session.viewport().contains(at("A15")));
    session.resize(40, 3);
    EXPECT_TRUE(session.viewport().contains(at("A15")));
}

TEST_F(SessionKeysTest, DeleteClearsCell) {
    press({"5", "enter", "up", "delete"});
    EXPECT_EQ(session.document().raw(at("A1")), "");
}

TEST_F(SessionKeysTest, HelpAndBack) {
    press({"?"});
    EXPECT_EQ(session.mode_name(), "help");
    // режим сменился изнутри HelpMode::handle_key
    press({"x"});
    EXPECT_EQ(session.mode_name(), "normal");
    // x закрыл справку, а не начал правку
    EXPECT_EQ(session.document().raw(at("A1")), "");
    press({"f1", "esc"});
    EXPECT_EQ(session.mode_name(), "normal");
}

TEST_F(SessionKeysTest, ModeSwitchesManyTimesInARow) {
    for (int i = 0; i < 50; ++i) {
        press({"?", "x", ":", "esc", "enter", "esc"});
    }
    EXPECT_EQ(session.mode_name(), "normal");
}

TEST_F(SessionKeysTest, OldModeLivesUntilItReturns) {
    std::string last_seen;
    session.set_mode(std::make_unique<SelfReplacingMode>("m", last_seen));
    press({"a", "b"});
    EXPECT_EQ(last_seen, "m+");
    EXPECT_EQ(session.mode_name(), "m++");
}

TEST_F(SessionKeysTest, SetModeOutsideOfKeyIsImmediate) {
    session.set_mode(std::make_unique<ss::app::HelpMode>());
    EXPECT_EQ(session.mode_name(), "help");
}

TEST_F(SessionKeysTest, CommandLineRunsAction) {
    press({"down", "down", ":"});
    EXPECT_EQ(session.mode_name(), "command");
    type("go-top");
    EXPECT_EQ(session.edit_buffer(), "go-top");
    press({"enter"});
    EXPECT_EQ(session.cursor(), at("A1"));
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_EQ(session.edit_buffer(), "");
}

TEST_F(SessionKeysTest, CommandLineCanSwitchMode) {
    press({":"});
    type("show-help");
    press({"enter"});
    EXPECT_EQ(session.mode_name(), "help");
}

TEST_F(SessionKeysTest, UnknownCommandKeepsLine) {
    press({":"});
    type("xyz");
    press({"enter"});
    EXPECT_EQ(session.mode_name(), "command");
    EXPECT_EQ(session.status().kind, Status::Kind::Error);
    EXPECT_EQ(session.edit_buffer(), "xyz");
    press({"esc"});
    EXPECT_EQ(session.mode_name(), "normal");
}

TEST_F(SessionKeysTest, CommandWithUnexpectedArgument) {
    press({":"});
    type("go-top 5");
    press({"enter"});
    EXPECT_EQ(session.status().kind, Status::Kind::Error);
    EXPECT_EQ(session.edit_buffer(), "go-top 5");
}

TEST_F(SessionKeysTest, EmptyCommandOrBackspaceLeaves) {
    press({":", "enter"});
    EXPECT_EQ(session.mode_name(), "normal");
    press({":", "backspace"});
    EXPECT_EQ(session.mode_name(), "normal");
}

TEST_F(SessionKeysTest, StatusIsClearedByNextKey) {
    press({":"});
    type("xyz");
    press({"enter", "esc"});
    EXPECT_EQ(session.status().text, "");
}

TEST_F(SessionKeysTest, UnboundKeyIsIgnored) {
    press({"f12", "ctrl+x", "esc"});
    EXPECT_EQ(session.mode_name(), "normal");
    EXPECT_EQ(session.status().kind, Status::Kind::Info);
}

TEST_F(SessionKeysTest, HelpListsEveryAction) {
    const std::vector<std::string> lines = session.help_lines();
    ASSERT_EQ(lines.size(), actions.names().size());
    for (const std::string& name : actions.names()) {
        const std::string description(actions.create(name)->description());
        const bool found = std::ranges::any_of(lines, [&](const std::string& line) {
            return line.starts_with(name) && line.ends_with(description);
        });
        EXPECT_TRUE(found) << name;
    }
    EXPECT_NE(std::ranges::find(lines, "move-up [k, up]: курсор вверх"), lines.end());
}

TEST_F(SessionKeysTest, ObserversAreNotifiedAfterEachKey) {
    CountingObserver observer;
    session.subscribe(&observer);
    session.subscribe(&observer);
    press({"1", "2", "enter"});
    session.resize(10, 10);
    EXPECT_EQ(observer.calls, 4);
    session.unsubscribe(&observer);
    press({"down"});
    EXPECT_EQ(observer.calls, 4);
}

TEST_F(SessionKeysTest, ObserverMayUnsubscribeDuringNotification) {
    LeavingObserver leaving(session);
    CountingObserver counting;
    session.subscribe(&leaving);
    session.subscribe(&counting);
    press({"down", "down"});
    EXPECT_EQ(leaving.calls, 1);
    EXPECT_EQ(counting.calls, 2);
    session.unsubscribe(&counting);
}
