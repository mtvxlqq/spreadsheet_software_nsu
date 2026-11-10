// Учебное расширение app: действие go-end. Новый класс, строка
// регистрации и клавиша в конфиге, существующий код не менялся (OCP).

#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "app/action.hpp"
#include "app/action_registry.hpp"
#include "app/errors.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/value_parser.hpp"
#include "support/manual_dispatcher.hpp"
#include "support/printers.hpp"

using ss::app::Session;
using ss::core::CellAddress;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

// 3. к последней заполненной строке, столбец не меняется
class GoEndAction final : public ss::app::Action {
public:
    std::string_view name() const override {
        return "go-end";
    }

    std::string_view description() const override {
        return "к последней заполненной строке";
    }

    void execute(Session& session, std::span<const std::string> args) const override {
        if (!args.empty()) {
            throw ss::app::CommandError("go-end не принимает аргументов");
        }
        const std::optional<ss::core::Range> used = session.document().used_range();
        if (!used) {
            return;  // лист пустой, идти некуда
        }
        session.set_cursor({.row = used->to().row, .col = session.cursor().col});
    }
};

ss::app::Keymap keymap_with_go_end(const ss::app::ActionRegistry& actions) {
    // так же, как строка в ~/.config/spreadsheet/keys.conf
    std::istringstream config("ctrl+end = go-end\n");
    ss::app::KeymapLoadResult result = ss::app::Keymap::load(config, actions.names());
    EXPECT_TRUE(result.warnings.empty());
    return std::move(result.keymap);
}

class GoEndTest : public ::testing::Test {
protected:
    GoEndTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
        ss::core::register_formats(formats);
        ss::core::register_number_formats(number_formats);
        ss::app::register_actions(actions);
        // та самая одна строка регистрации
        actions.add("go-end", [] { return std::make_unique<GoEndAction>(); });
        session = std::make_unique<Session>(functions, value_parsers, formats, number_formats, actions,
                                            keymap_with_go_end(actions), dispatcher);
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    ss::core::FormatRegistry formats;
    ss::core::NumberFormatRegistry number_formats;
    ss::app::ActionRegistry actions;
    ss::test::ManualDispatcher dispatcher;
    std::unique_ptr<Session> session;
};

}  // namespace

TEST_F(GoEndTest, KeyGoesToLastFilledRow) {
    session->resize(10, 50);
    session->set_cursor(at("C3"));
    session->handle_key("1");
    session->handle_key("enter");
    session->set_cursor(at("A40"));
    session->handle_key("2");
    session->handle_key("enter");

    session->set_cursor(at("B1"));
    session->handle_key("ctrl+end");
    EXPECT_EQ(session->cursor(), at("B40"));
    // экран прокрутился за курсором
    EXPECT_TRUE(session->viewport().contains(at("B40"), session->document().column_widths()));
}

TEST_F(GoEndTest, EmptySheetStaysInPlace) {
    session->set_cursor(at("D4"));
    session->handle_key("ctrl+end");
    EXPECT_EQ(session->cursor(), at("D4"));
}

TEST_F(GoEndTest, WorksFromCommandLineAndHelp) {
    session->set_cursor(at("A7"));
    session->handle_key("x");
    session->handle_key("enter");
    session->set_cursor(at("A1"));
    for (const char* key : {":", "g", "o", "-", "e", "n", "d", "enter"}) {
        session->handle_key(key);
    }
    EXPECT_EQ(session->cursor(), at("A7"));

    // в справку действие попало само, вместе со своей клавишей
    bool found = false;
    for (const std::string& line : session->help_lines()) {
        found = found || line == "go-end [ctrl+end]: к последней заполненной строке";
    }
    EXPECT_TRUE(found);
}
