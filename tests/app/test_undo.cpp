#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "app/action_registry.hpp"
#include "app/edit_command.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "app/set_cell_command.hpp"
#include "app/undo_stack.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_style.hpp"
#include "core/document_edit.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "support/manual_dispatcher.hpp"

using ss::app::Session;
using ss::app::SetCellCommand;
using ss::app::Status;
using ss::app::UndoStack;
using ss::core::CellAddress;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class UndoStackTest : public ::testing::Test {
protected:
    UndoStackTest() {
        ss::core::register_builtins(functions);
    }

    void set(UndoStack& stack, std::string_view address, std::string raw) {
        auto command = std::make_unique<SetCellCommand>(at(address), std::move(raw));
        command->apply(document);
        stack.push(std::move(command));
    }

    ss::core::FunctionRegistry functions;
    ss::core::Spreadsheet document{functions};
};

class UndoSessionTest : public ::testing::Test {
protected:
    UndoSessionTest() {
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

    std::string raw(std::string_view address) const {
        return session.document().raw(at(address));
    }

    ss::core::FunctionRegistry functions;
    ss::core::FormatRegistry formats;
    ss::app::ActionRegistry actions;
    ss::test::ManualDispatcher dispatcher;
    Session session{functions, formats, actions, ss::app::Keymap::defaults(), dispatcher};
};

}  // namespace

TEST_F(UndoStackTest, EmptyStackHasNothingToDo) {
    UndoStack stack;
    EXPECT_FALSE(stack.can_undo());
    EXPECT_FALSE(stack.can_redo());
    EXPECT_FALSE(stack.undo(document));
    EXPECT_FALSE(stack.redo(document));
}

TEST_F(UndoStackTest, UndoAndRedoGoInOrder) {
    UndoStack stack;
    set(stack, "A1", "1");
    set(stack, "A1", "2");
    set(stack, "A1", "3");

    EXPECT_TRUE(stack.undo(document));
    EXPECT_EQ(document.raw(at("A1")), "2");
    EXPECT_TRUE(stack.undo(document));
    EXPECT_EQ(document.raw(at("A1")), "1");
    EXPECT_TRUE(stack.redo(document));
    EXPECT_EQ(document.raw(at("A1")), "2");
    EXPECT_TRUE(stack.redo(document));
    EXPECT_EQ(document.raw(at("A1")), "3");
    EXPECT_FALSE(stack.redo(document));
}

TEST_F(UndoStackTest, NewEditForgetsRedo) {
    UndoStack stack;
    set(stack, "A1", "1");
    set(stack, "A1", "2");
    ASSERT_TRUE(stack.undo(document));
    set(stack, "B1", "5");
    EXPECT_FALSE(stack.can_redo());
    EXPECT_EQ(document.raw(at("A1")), "1");
}

TEST_F(UndoStackTest, OldestEditsAreForgotten) {
    UndoStack stack(2);
    set(stack, "A1", "1");
    set(stack, "A1", "2");
    set(stack, "A1", "3");
    EXPECT_TRUE(stack.undo(document));
    EXPECT_TRUE(stack.undo(document));
    EXPECT_FALSE(stack.undo(document));
    EXPECT_EQ(document.raw(at("A1")), "1");
}

TEST_F(UndoStackTest, ClearForgetsEverything) {
    UndoStack stack;
    set(stack, "A1", "1");
    set(stack, "A1", "2");
    ASSERT_TRUE(stack.undo(document));
    stack.clear();
    EXPECT_FALSE(stack.can_undo());
    EXPECT_FALSE(stack.can_redo());
}

TEST_F(UndoStackTest, UndoRecalculatesDependents) {
    UndoStack stack;
    set(stack, "A1", "2");
    set(stack, "B1", "=A1*10");
    set(stack, "A1", "5");
    EXPECT_EQ(document.value(at("B1"))->to_text(), "50");
    ASSERT_TRUE(stack.undo(document));
    EXPECT_EQ(document.value(at("B1"))->to_text(), "20");
}

TEST_F(UndoStackTest, UndoKeepsStyleOfCell) {
    const ss::core::CellStyle style{.format = "fixed:2", .align = ss::core::Align::Right};
    document.apply({.cells = {{.address = at("A1"), .before = {}, .after = {.raw = "", .style = style}}}});
    UndoStack stack;
    set(stack, "A1", "7");
    EXPECT_EQ(document.style(at("A1")), style);
    ASSERT_TRUE(stack.undo(document));
    EXPECT_EQ(document.raw(at("A1")), "");
    EXPECT_EQ(document.style(at("A1")), style);
}

TEST_F(UndoSessionTest, CtrlZUndoesTypedValue) {
    press({"1", "enter"});
    press({"up", "2", "enter"});
    EXPECT_EQ(raw("A1"), "2");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "1");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "");
    press({"ctrl+y"});
    EXPECT_EQ(raw("A1"), "1");
}

TEST_F(UndoSessionTest, UndoClearCell) {
    press({"5", "enter", "up", "delete"});
    EXPECT_EQ(raw("A1"), "");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "5");
}

TEST_F(UndoSessionTest, NothingToUndoIsReported) {
    press({"ctrl+z"});
    EXPECT_EQ(session.status().text, "нечего отменять");
    press({"ctrl+y"});
    EXPECT_EQ(session.status().text, "нечего повторять");
    EXPECT_FALSE(session.dirty());
}

TEST_F(UndoSessionTest, BrokenFormulaIsNotInHistory) {
    press({"1", "enter", "up", "=", "1", "+", "enter", "esc"});
    EXPECT_EQ(raw("A1"), "1");
    press({"ctrl+z"});
    EXPECT_EQ(raw("A1"), "");
    press({"ctrl+z"});
    EXPECT_EQ(session.status().text, "нечего отменять");
}

TEST_F(UndoSessionTest, UndoAfterSaveMakesDocumentDirty) {
    press({"1", "enter"});
    session.mark_saved("saved.sst");
    press({"ctrl+z"});
    EXPECT_TRUE(session.dirty());
}

TEST_F(UndoSessionTest, NewDocumentForgetsHistory) {
    press({"1", "enter"});
    session.import_document(std::make_unique<ss::core::Spreadsheet>(functions));
    press({"ctrl+z"});
    EXPECT_EQ(session.status().text, "нечего отменять");
}

TEST_F(UndoSessionTest, CommandLineUndo) {
    press({"1", "enter"});
    press({":", "u", "enter"});
    EXPECT_EQ(raw("A1"), "");
    press({":", "r", "e", "d", "o", "enter"});
    EXPECT_EQ(raw("A1"), "1");
}
