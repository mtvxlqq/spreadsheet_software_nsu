#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <gtest/gtest.h>

#include "app/keymap.hpp"
#include "tui/key_names.hpp"

using ftxui::Event;
using ss::tui::key_name;

namespace {

std::vector<std::pair<Event, std::string>> named_events() {
    return {
        {Event::ArrowUp, "up"},
        {Event::ArrowDown, "down"},
        {Event::ArrowLeft, "left"},
        {Event::ArrowRight, "right"},
        {Event::Return, "enter"},
        {Event::Escape, "esc"},
        {Event::Backspace, "backspace"},
        {Event::Delete, "delete"},
        {Event::Tab, "tab"},
        {Event::Home, "home"},
        {Event::End, "end"},
        {Event::PageUp, "pageup"},
        {Event::PageDown, "pagedown"},
        {Event::F1, "f1"},
        {Event::F2, "f2"},
        {Event::F3, "f3"},
        {Event::F4, "f4"},
        {Event::F5, "f5"},
        {Event::F6, "f6"},
        {Event::F7, "f7"},
        {Event::F8, "f8"},
        {Event::F9, "f9"},
        {Event::F10, "f10"},
        {Event::F11, "f11"},
        {Event::F12, "f12"},
        {Event::ArrowUpCtrl, "ctrl+up"},
        {Event::ArrowDownCtrl, "ctrl+down"},
        {Event::ArrowLeftCtrl, "ctrl+left"},
        {Event::ArrowRightCtrl, "ctrl+right"},
        {Event::Special("\x1B[1;5H"), "ctrl+home"},
        {Event::Special("\x1B[1;5F"), "ctrl+end"},
    };
}

}  // namespace

TEST(KeyNamesTest, NamedKeys) {
    for (const auto& [event, name] : named_events()) {
        EXPECT_EQ(key_name(event), name);
    }
}

TEST(KeyNamesTest, CtrlLetters) {
    EXPECT_EQ(key_name(Event::CtrlA), "ctrl+a");
    EXPECT_EQ(key_name(Event::CtrlS), "ctrl+s");
    EXPECT_EQ(key_name(Event::CtrlQ), "ctrl+q");
    EXPECT_EQ(key_name(Event::CtrlZ), "ctrl+z");
}

// у них те же байты, что у Tab и Enter
TEST(KeyNamesTest, TabAndEnterWinOverCtrlLetters) {
    EXPECT_EQ(key_name(Event::CtrlI), "tab");
    EXPECT_EQ(key_name(Event::CtrlJ), "enter");
}

TEST(KeyNamesTest, PlainCharacters) {
    EXPECT_EQ(key_name(Event::Character('h')), "h");
    EXPECT_EQ(key_name(Event::Character('=')), "=");
    EXPECT_EQ(key_name(Event::Character(' ')), " ");
}

TEST(KeyNamesTest, CharacterCaseIsKept) {
    EXPECT_EQ(key_name(Event::Character('H')), "H");
    EXPECT_EQ(key_name(Event::Character('h')), "h");
}

TEST(KeyNamesTest, CyrillicIsOneCharacter) {
    EXPECT_EQ(key_name(Event::Character("ж")), "ж");
    EXPECT_EQ(key_name(Event::Character("Ж")), "Ж");
}

TEST(KeyNamesTest, SeveralCharactersAreNotAKey) {
    EXPECT_EQ(key_name(Event::Character("ab")), std::nullopt);
}

TEST(KeyNamesTest, CustomEventIsNotAKey) {
    EXPECT_EQ(key_name(Event::Custom), std::nullopt);
}

TEST(KeyNamesTest, MouseIsNotAKey) {
    EXPECT_EQ(key_name(Event::Mouse("\x1B[<0;1;1M", ftxui::Mouse{})), std::nullopt);
}

// такой ответ на запрос о курсоре приходит при запуске в tmux
TEST(KeyNamesTest, TerminalReplyIsNotAKey) {
    EXPECT_EQ(key_name(Event::Special("\x1BP1$r q0 q\x1B\\")), std::nullopt);
}

TEST(KeyNamesTest, AltIsNotAKey) {
    EXPECT_EQ(key_name(Event::AltA), std::nullopt);
}

// имена должны понимать и Keymap, и его конфиг
TEST(KeyNamesTest, EveryNamePassesNormalizeKey) {
    for (const auto& [event, name] : named_events()) {
        EXPECT_EQ(ss::app::Keymap::normalize_key(*key_name(event)), name);
    }
    for (const Event& event : {Event::CtrlA, Event::CtrlS, Event::Character('h'),
                               Event::Character('='), Event::Character("ж")}) {
        const std::optional<std::string> name = key_name(event);
        ASSERT_TRUE(name.has_value());
        EXPECT_EQ(ss::app::Keymap::normalize_key(*name), name);
    }
}
