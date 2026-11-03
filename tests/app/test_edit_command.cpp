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
#include "core/number_format.hpp"
#include "core/spreadsheet.hpp"

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
    }

    ss::core::FunctionRegistry functions;
    ss::core::Spreadsheet document{functions};
    ss::core::GeneralFormat format;
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
    EXPECT_EQ(CellPresenter::align(document, at("A1")), Align::Right);
    EXPECT_EQ(CellPresenter::align(document, at("A2")), Align::Left);
    EXPECT_EQ(CellPresenter::align(document, at("A3")), Align::Left);
    EXPECT_EQ(CellPresenter::align(document, at("A4")), Align::Right);
    EXPECT_EQ(CellPresenter::align(document, at("Z9")), Align::Left);
}

TEST_F(DocumentTest, PresenterText) {
    document.set_input(at("A1"), "=1/3");
    document.set_input(at("A2"), "'0012");
    document.set_input(at("A3"), "=1/0");
    EXPECT_EQ(CellPresenter::text(document, at("A1"), format), "0.3333333333");
    EXPECT_EQ(CellPresenter::text(document, at("A2"), format), "0012");
    EXPECT_EQ(CellPresenter::text(document, at("A3"), format), "#DIV/0!");
    EXPECT_EQ(CellPresenter::text(document, at("Z9"), format), "");
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
