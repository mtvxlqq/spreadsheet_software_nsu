// Учебные расширения app: действие go-end и правка с отменой swap-right.
// Новый класс, строка регистрации и клавиша в конфиге, существующий код
// не менялся (OCP).

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
#include "app/planned_command.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/document_edit.hpp"
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

// 1. к последней заполненной строке, столбец не меняется
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

// 2. Новая операция с отменой: поменять две ячейки местами вместе
// с оформлением. От PlannedCommand нужен только plan, отмену и повтор
// даёт базовый класс.
class SwapCellsCommand final : public ss::app::PlannedCommand {
public:
    SwapCellsCommand(CellAddress first, CellAddress second) : first_(first), second_(second) {}

private:
    ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const override {
        const ss::core::CellContent first = document.content(first_);
        const ss::core::CellContent second = document.content(second_);
        ss::core::DocumentEdit edit;
        edit.cells.push_back({.address = first_, .before = first, .after = second});
        edit.cells.push_back({.address = second_, .before = second, .after = first});
        return edit;
    }

    CellAddress first_;
    CellAddress second_;
};

// действие, которое запускает команду: ячейка под курсором меняется с правой
class SwapRightAction final : public ss::app::Action {
public:
    std::string_view name() const override {
        return "swap-right";
    }

    std::string_view description() const override {
        return "поменять ячейку местами с правой";
    }

    void execute(Session& session, std::span<const std::string> /*args*/) const override {
        const CellAddress here = session.cursor();
        session.apply_edit(std::make_unique<SwapCellsCommand>(
            here, CellAddress{.row = here.row, .col = here.col + 1}));
    }
};

ss::app::Keymap keymap_with_go_end(const ss::app::ActionRegistry& actions) {
    // так же, как строки в ~/.config/spreadsheet/keys.conf
    std::istringstream config("ctrl+end = go-end\nf9 = swap-right\n");
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
        actions.add("swap-right", [] { return std::make_unique<SwapRightAction>(); });
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

TEST_F(GoEndTest, SwapCommandIsUndoneLikeBuiltinOnes) {
    session->set_cursor(at("A1"));
    session->handle_key("1");
    session->handle_key("tab");
    for (const char* key : {"=", "A", "1", "*", "2", "enter"}) {
        session->handle_key(key);
    }
    // у A1 своё оформление, оно тоже должно переехать
    session->set_cursor(at("A1"));
    for (const char* key : {":", "a", "l", "i", "g", "n", " ", "c", "e", "n", "t", "e", "r", "enter"}) {
        session->handle_key(key);
    }

    session->handle_key("f9");
    const ss::core::Spreadsheet& document = session->document();
    EXPECT_EQ(document.raw(at("A1")), "=A1*2");
    EXPECT_EQ(document.raw(at("B1")), "1");
    EXPECT_EQ(document.style(at("B1")).align, ss::core::Align::Center);
    EXPECT_EQ(document.style(at("A1")), ss::core::CellStyle{});
    // формула теперь ссылается сама на себя
    EXPECT_EQ(document.value(at("A1"))->to_text(), "#CYCLE!");

    session->handle_key("ctrl+z");
    EXPECT_EQ(document.raw(at("A1")), "1");
    EXPECT_EQ(document.raw(at("B1")), "=A1*2");
    EXPECT_EQ(document.style(at("A1")).align, ss::core::Align::Center);
    EXPECT_EQ(document.value(at("B1"))->to_text(), "2");

    session->handle_key("ctrl+y");
    EXPECT_EQ(document.raw(at("B1")), "1");
}

// у правого края листа соседа нет: ошибка в статусе, документ цел
TEST_F(GoEndTest, SwapAtSheetEdgeChangesNothing) {
    session->set_cursor(at("ZZ1"));
    session->handle_key("7");
    session->handle_key("enter");
    session->set_cursor(at("ZZ1"));
    session->handle_key("f9");
    EXPECT_EQ(session->status().kind, ss::app::Status::Kind::Error);
    EXPECT_EQ(session->document().raw(at("ZZ1")), "7");
}
