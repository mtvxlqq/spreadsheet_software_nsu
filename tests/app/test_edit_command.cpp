#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "app/action.hpp"
#include "app/action_registry.hpp"
#include "app/cell_presenter.hpp"
#include "app/errors.hpp"
#include "app/set_cell_command.hpp"
#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/value_parser.hpp"

using ss::app::Align;
using ss::app::CellPresenter;
using ss::app::SetCellCommand;
using ss::core::CellAddress;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class DocumentTest : public ::testing::Test {
protected:
    DocumentTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
        ss::core::register_number_formats(formats);
    }

    ss::app::CellView present(std::string_view address) const {
        return CellPresenter::present(document, at(address), formats);
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    ss::core::NumberFormatRegistry formats;
    ss::core::Spreadsheet document{functions, value_parsers};
};

// ничего не делает, нужно только для проверки реестра
class DummyAction final : public ss::app::Action {
public:
    explicit DummyAction(std::string name) : name_(std::move(name)) {}

    std::string_view name() const override {
        return name_;
    }

    std::string_view description() const override {
        return "dummy";
    }

    void execute(ss::app::Session& /*session*/,
                 std::span<const std::string> /*args*/) const override {}

private:
    std::string name_;
};

ss::app::ActionRegistry::Factory dummy(const std::string& name) {
    return [name] { return std::make_unique<DummyAction>(name); };
}

}  // namespace

TEST_F(DocumentTest, SetCellCommandAppliesAndReverts) {
    document.set_input(at("A1"), "old");
    SetCellCommand command(at("A1"), "=1+2");
    command.apply(document);
    EXPECT_EQ(document.value(at("A1"))->to_text(), "3");
    command.revert(document);
    EXPECT_EQ(document.raw(at("A1")), "old");
}

TEST_F(DocumentTest, RevertOfNewCellClearsIt) {
    SetCellCommand command(at("B2"), "5");
    command.apply(document);
    command.revert(document);
    EXPECT_EQ(document.raw(at("B2")), "");
    EXPECT_FALSE(document.used_range().has_value());
}

TEST_F(DocumentTest, BrokenFormulaChangesNothing) {
    document.set_input(at("A1"), "7");
    SetCellCommand command(at("A1"), "=1+");
    EXPECT_THROW(command.apply(document), ss::core::ParseError);
    EXPECT_EQ(document.raw(at("A1")), "7");
}

TEST_F(DocumentTest, PresenterAlignsNumbersRight) {
    document.set_input(at("A1"), "42");
    document.set_input(at("A2"), "text");
    document.set_input(at("A3"), "=1/0");
    document.set_input(at("A4"), "=A1*2");
    EXPECT_EQ(present("A1").align, Align::Right);
    EXPECT_EQ(present("A2").align, Align::Left);
    EXPECT_EQ(present("A3").align, Align::Left);
    EXPECT_EQ(present("A4").align, Align::Right);
    EXPECT_EQ(present("Z9").align, Align::Left);
    // решётками при нехватке места рисуются только числа
    EXPECT_TRUE(present("A1").is_number);
    EXPECT_FALSE(present("A2").is_number);
    EXPECT_FALSE(present("A3").is_number);
    EXPECT_FALSE(present("Z9").is_number);
}

TEST_F(DocumentTest, PresenterText) {
    document.set_input(at("A1"), "=1/3");
    document.set_input(at("A2"), "'0012");
    document.set_input(at("A3"), "=1/0");
    EXPECT_EQ(present("A1").text, "0.3333333333");
    EXPECT_EQ(present("A2").text, "0012");
    EXPECT_EQ(present("A3").text, "#DIV/0!");
    EXPECT_EQ(present("Z9").text, "");
}

TEST_F(DocumentTest, PresenterUsesStyleOfCell) {
    document.set_input(at("A1"), "=1/3");
    document.set_input(at("A2"), "text");
    ss::core::DocumentEdit edit;
    for (const char* address : {"A1", "A2"}) {
        ss::core::CellContent after = document.content(at(address));
        after.style = {.format = "percent:1", .align = Align::Center};
        edit.cells.push_back(
            {.address = at(address), .before = document.content(at(address)), .after = after});
    }
    document.apply(edit);

    EXPECT_EQ(present("A1").text, "33.3%");
    EXPECT_EQ(present("A1").align, Align::Center);
    // формат чисел на текст не действует, а выравнивание действует
    EXPECT_EQ(present("A2").text, "text");
    EXPECT_EQ(present("A2").align, Align::Center);
}

// файл мог сохранить кто-то с другим набором форматов
TEST_F(DocumentTest, PresenterShowsUnknownFormatAsGeneral) {
    document.set_input(at("A1"), "0.5");
    ss::core::CellContent after = document.content(at("A1"));
    after.style.format = "money:2";
    ss::core::DocumentEdit edit;
    edit.cells.push_back({.address = at("A1"), .before = document.content(at("A1")), .after = after});
    document.apply(edit);
    EXPECT_EQ(present("A1").text, "0.5");
    EXPECT_EQ(document.style(at("A1")).format, "money:2");
}

TEST(ActionRegistryTest, CreatesByNameAndAlias) {
    ss::app::ActionRegistry registry;
    registry.add("save", dummy("save"));
    registry.add_alias("w", "save");
    EXPECT_EQ(registry.create("save")->name(), "save");
    EXPECT_EQ(registry.create("w")->name(), "save");
}

TEST(ActionRegistryTest, UnknownNameIsCommandError) {
    const ss::app::ActionRegistry registry;
    EXPECT_THROW(static_cast<void>(registry.create("nope")), ss::app::CommandError);
}

TEST(ActionRegistryTest, NamesAreSortedWithoutAliases) {
    ss::app::ActionRegistry registry;
    registry.add("quit", dummy("quit"));
    registry.add("open", dummy("open"));
    registry.add_alias("q", "quit");
    EXPECT_EQ(registry.names(), (std::vector<std::string>{"open", "quit"}));
}

TEST(ActionRegistryTest, NameClashesAreRejected) {
    ss::app::ActionRegistry registry;
    registry.add("save", dummy("save"));
    registry.add_alias("w", "save");
    EXPECT_THROW(registry.add("save", dummy("save")), ss::core::Error);
    EXPECT_THROW(registry.add("w", dummy("w")), ss::core::Error);
    EXPECT_THROW(registry.add_alias("w", "save"), ss::core::Error);
    EXPECT_THROW(registry.add_alias("x", "nope"), ss::core::Error);
}
