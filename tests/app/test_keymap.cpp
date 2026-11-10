#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "app/errors.hpp"
#include "app/keymap.hpp"

using ss::app::Keymap;
using ss::app::KeymapLoadResult;

namespace {

// В фазе 11 этот список будет давать ActionRegistry::names()
const std::vector<std::string> known_actions = {
    "move-up",   "move-down",  "move-left",  "move-right", "page-up",   "page-down",
    "go-home",   "go-top",     "start-edit", "clear-cell", "command-mode", "show-help",
    "save",      "open",       "import-csv", "quit",       "force-quit",   "undo",
    "redo",
};

std::string testdata(const std::string& name) {
    return std::string(SS_TESTDATA_DIR) + "/config/" + name;
}

KeymapLoadResult load_text(const std::string& text) {
    std::istringstream in(text);
    return Keymap::load(in, known_actions);
}

}  // namespace

TEST(KeymapTest, Defaults) {
    const Keymap keymap = Keymap::defaults();
    EXPECT_EQ(keymap.action_for("up"), "move-up");
    EXPECT_EQ(keymap.action_for("k"), "move-up");
    EXPECT_EQ(keymap.action_for("ctrl+home"), "go-top");
    EXPECT_EQ(keymap.action_for(":"), "command-mode");
    EXPECT_EQ(keymap.action_for("f1"), "show-help");
    EXPECT_EQ(keymap.action_for("ctrl+q"), "quit");
    EXPECT_EQ(keymap.action_for("x"), std::nullopt);
}

TEST(KeymapTest, DefaultsUseOnlyKnownActions) {
    const Keymap keymap = Keymap::defaults();
    for (const char* key : {"up", "down", "left", "right", "h", "j", "k", "l", "pageup",
                            "pagedown", "home", "ctrl+home", "enter", "f2", "delete", ":", "?",
                            "f1", "ctrl+s", "ctrl+q"}) {
        const std::optional<std::string> action = keymap.action_for(key);
        ASSERT_TRUE(action.has_value()) << key;
        EXPECT_NE(std::ranges::find(known_actions, *action), known_actions.end()) << key;
    }
}

TEST(KeymapTest, NormalizesKeyNames) {
    EXPECT_EQ(Keymap::normalize_key("PageUp"), "pageup");
    EXPECT_EQ(Keymap::normalize_key("Ctrl+S"), "ctrl+s");
    EXPECT_EQ(Keymap::normalize_key("CTRL+Home"), "ctrl+home");
    EXPECT_EQ(Keymap::normalize_key("F12"), "f12");
    EXPECT_EQ(Keymap::normalize_key("ctrl++"), "ctrl++");
    // у одиночного символа регистр важен
    EXPECT_EQ(Keymap::normalize_key("H"), "H");
    EXPECT_EQ(Keymap::normalize_key("ж"), "ж");
}

TEST(KeymapTest, RejectsUnknownKeys) {
    for (const char* key : {"", "ctrl+", "f0", "f13", "hello", "ctrl+hello", "ab", "\x01",
                            "\xD0", "\xD0\x41"}) {
        EXPECT_EQ(Keymap::normalize_key(key), std::nullopt) << key;
    }
    Keymap keymap;
    EXPECT_THROW(keymap.bind("hello", "save"), ss::app::ConfigError);
}

TEST(KeymapTest, BindReplacesOldAction) {
    Keymap keymap = Keymap::defaults();
    keymap.bind("H", "go-home");
    keymap.bind("h", "save");
    EXPECT_EQ(keymap.action_for("h"), "save");
    EXPECT_EQ(keymap.action_for("H"), "go-home");
    EXPECT_EQ(keymap.action_for("Ctrl+S"), "save");
}

TEST(KeymapTest, KeysForAction) {
    const Keymap keymap = Keymap::defaults();
    EXPECT_EQ(keymap.keys_for("move-up"), (std::vector<std::string>{"k", "up"}));
    EXPECT_EQ(keymap.keys_for("show-help"), (std::vector<std::string>{"?", "f1"}));
    EXPECT_TRUE(keymap.keys_for("force-quit").empty());
}

TEST(KeymapTest, LoadsGoodFile) {
    const KeymapLoadResult result = Keymap::load(testdata("keys_ok.conf"), known_actions);
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_EQ(result.keymap.action_for("ctrl+w"), "save");
    EXPECT_EQ(result.keymap.action_for("H"), "go-home");
    EXPECT_EQ(result.keymap.action_for("x"), "clear-cell");
    EXPECT_EQ(result.keymap.action_for("="), "command-mode");
    // то, чего нет в файле, осталось по умолчанию
    EXPECT_EQ(result.keymap.action_for("ctrl+s"), "save");
    EXPECT_EQ(result.keymap.action_for("h"), "move-left");
}

TEST(KeymapTest, BrokenFileGivesWarningsAndKeepsDefaults) {
    const KeymapLoadResult result = Keymap::load(testdata("keys_broken.conf"), known_actions);
    ASSERT_EQ(result.warnings.size(), 5U);
    EXPECT_TRUE(result.warnings[0].starts_with("строка 2: ")) << result.warnings[0];
    EXPECT_TRUE(result.warnings[1].starts_with("строка 3: ")) << result.warnings[1];
    EXPECT_TRUE(result.warnings[2].starts_with("строка 4: ")) << result.warnings[2];
    EXPECT_EQ(result.warnings[3], "строка 5: неизвестное действие fly-away");
    EXPECT_EQ(result.warnings[4], "строка 6: непонятная клавиша ctrl+");

    EXPECT_EQ(result.keymap.action_for("h"), "move-left");
    EXPECT_EQ(result.keymap.action_for("ctrl+s"), "save");
    EXPECT_EQ(result.keymap.action_for("q"), std::nullopt);
    // хорошая строка после плохих всё равно применилась
    EXPECT_EQ(result.keymap.action_for("z"), "clear-cell");
}

TEST(KeymapTest, MissingFileGivesDefaultsSilently) {
    const KeymapLoadResult result = Keymap::load(testdata("no_such_file.conf"), known_actions);
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_EQ(result.keymap.action_for("up"), "move-up");
}

TEST(KeymapTest, DirectoryInsteadOfFileIsWarned) {
    const KeymapLoadResult result = Keymap::load(SS_TESTDATA_DIR, known_actions);
    EXPECT_EQ(result.warnings.size(), 1U);
    EXPECT_EQ(result.keymap.action_for("up"), "move-up");
}

TEST(KeymapTest, CommentsAndBlankLines) {
    const KeymapLoadResult result = load_text("# comment\n\n   \n  # indented\r\nx = save\r\n");
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_EQ(result.keymap.action_for("x"), "save");
}

TEST(KeymapTest, LaterLineWins) {
    const KeymapLoadResult result = load_text("x = save\nx = quit\n");
    EXPECT_EQ(result.keymap.action_for("x"), "quit");
}

TEST(KeymapTest, ShippedConfigMatchesDefaults) {
    const KeymapLoadResult result =
        Keymap::load(std::string(SS_TESTDATA_DIR) + "/../config/keys.conf", known_actions);
    EXPECT_TRUE(result.warnings.empty());
    const Keymap defaults = Keymap::defaults();
    for (const std::string& action : known_actions) {
        EXPECT_EQ(result.keymap.keys_for(action), defaults.keys_for(action)) << action;
    }
}
