#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "app/status.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/text.hpp"
#include "core/value.hpp"
#include "support/manual_dispatcher.hpp"

namespace ss::test {

inline core::CellAddress at(std::string_view text) {
    return core::CellAddress::parse(text);
}

// Сеанс со всеми настоящими реестрами, как в main. Тест нажимает клавиши
// и набирает команды так же, как это делал бы пользователь.
class SessionTest : public ::testing::Test {
protected:
    SessionTest() {
        core::register_builtins(functions);
        core::register_formats(formats);
        core::register_number_formats(number_formats);
        app::register_actions(actions);
        session.resize(20, 80);
    }

    void press(std::initializer_list<std::string_view> keys) {
        for (const std::string_view key : keys) {
            session.handle_key(key);
        }
    }

    // по клавише на символ, русские буквы тоже можно
    void type(std::string_view text) {
        std::size_t i = 0;
        while (i < text.size()) {
            const std::size_t length = core::utf8_char_length(static_cast<unsigned char>(text[i]));
            session.handle_key(text.substr(i, length));
            i += length;
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
    }

    // записать raw в ячейку через правку, курсор после этого на строку ниже
    void input(std::string_view address, std::string_view raw) {
        session.set_cursor(at(address));
        // enter, а не первый символ: "k" из "keep" это move-up
        session.handle_key("enter");
        session.edit_line().clear();
        type(raw);
        session.handle_key("enter");
    }

    void go(std::string_view address) {
        session.set_cursor(at(address));
    }

    std::string raw(std::string_view address) const {
        return session.document().raw(at(address));
    }

    // значение ячейки без оформления
    std::string shown(std::string_view address) const {
        return session.document().value(at(address))->to_text();
    }

    bool failed() const {
        return session.status().kind == app::Status::Kind::Error;
    }

    core::FunctionRegistry functions;
    core::FormatRegistry formats;
    core::NumberFormatRegistry number_formats;
    app::ActionRegistry actions;
    ManualDispatcher dispatcher;
    app::Session session{functions, formats, number_formats, actions,
                         app::Keymap::defaults(), dispatcher};
};

}  // namespace ss::test
