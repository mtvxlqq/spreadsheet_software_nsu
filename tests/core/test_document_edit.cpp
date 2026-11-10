#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_observer.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/document_edit.hpp"
#include "core/errors.hpp"
#include "core/function.hpp"
#include "core/function_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "core/value_parser.hpp"

using ss::core::Align;
using ss::core::CellAddress;
using ss::core::CellChange;
using ss::core::CellContent;
using ss::core::CellStyle;
using ss::core::ColumnWidths;
using ss::core::DocumentEdit;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

CellContent input(std::string raw, CellStyle style = {}) {
    return {.raw = std::move(raw), .style = std::move(style)};
}

// Пока флаг поднят, бросает при вычислении. Так проверяем откат: по-настоящему
// посреди apply бросить может только нехватка памяти, её в тесте не устроить.
class FailingFunction final : public ss::core::Function {
public:
    explicit FailingFunction(const bool& fail) : fail_(fail) {}

    std::string_view name() const override {
        return "FAIL";
    }

    ss::core::ValuePtr call(std::span<const ss::core::Expr* const> /*args*/,
                            ss::core::EvalContext& /*context*/) const override {
        if (fail_) {
            throw std::runtime_error("как будто кончилась память");
        }
        return ss::core::make_number(1);
    }

private:
    const bool& fail_;
};

class CountingObserver final : public ss::core::CellObserver {
public:
    void on_cells_changed(std::span<const CellAddress> /*cells*/) override {
        ++calls;
    }

    int calls = 0;
};

class DocumentEditTest : public ::testing::Test {
protected:
    DocumentEditTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
        functions.add("FAIL", [&flag = fail] { return std::make_shared<FailingFunction>(flag); });
    }

    // правка, которая переводит ячейки из того, что сейчас, в after
    DocumentEdit edit_to(std::initializer_list<std::pair<std::string_view, CellContent>> cells) {
        DocumentEdit edit;
        for (const auto& [address, after] : cells) {
            edit.cells.push_back(
                {.address = at(address), .before = document.content(at(address)), .after = after});
        }
        return edit;
    }

    std::string shown(std::string_view address) const {
        return document.value(at(address))->to_text();
    }

    bool fail = false;
    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    ss::core::Spreadsheet document{functions, value_parsers};
};

const CellStyle right_two_digits{.format = "fixed:2", .align = Align::Right};

}  // namespace

TEST_F(DocumentEditTest, AppliesSeveralCells) {
    document.apply(edit_to({{"A1", input("1")}, {"A2", input("=A1+1")}}));
    EXPECT_EQ(shown("A1"), "1");
    EXPECT_EQ(shown("A2"), "2");
}

TEST_F(DocumentEditTest, OrderDoesNotMatterForDependents) {
    document.apply(edit_to({{"A2", input("=A1*10")}, {"A1", input("4")}}));
    EXPECT_EQ(shown("A2"), "40");
}

TEST_F(DocumentEditTest, DependentsOutsideEditAreRecalculated) {
    document.set_input(at("B1"), "=A1+A2");
    document.apply(edit_to({{"A1", input("1")}, {"A2", input("2")}}));
    EXPECT_EQ(shown("B1"), "3");
}

TEST_F(DocumentEditTest, NotifiesOnce) {
    CountingObserver observer;
    document.subscribe(&observer);
    document.apply(edit_to({{"A1", input("1")}, {"A2", input("2")}, {"A3", input("3")}}));
    EXPECT_EQ(observer.calls, 1);
    document.unsubscribe(&observer);
}

TEST_F(DocumentEditTest, EmptyEditDoesNothing) {
    CountingObserver observer;
    document.subscribe(&observer);
    document.apply(DocumentEdit{});
    EXPECT_EQ(observer.calls, 0);
    document.unsubscribe(&observer);
}

TEST_F(DocumentEditTest, EmptyRawClearsCell) {
    document.set_input(at("A1"), "5");
    document.apply(edit_to({{"A1", input("")}}));
    EXPECT_EQ(document.raw(at("A1")), "");
    EXPECT_FALSE(document.used_range().has_value());
}

TEST_F(DocumentEditTest, StyleIsStoredSeparately) {
    document.apply(edit_to({{"A1", input("5", right_two_digits)}}));
    EXPECT_EQ(document.style(at("A1")), right_two_digits);
    EXPECT_EQ(shown("A1"), "5");
}

TEST_F(DocumentEditTest, StyleOfEmptyCellIsKept) {
    document.apply(edit_to({{"B2", input("", right_two_digits)}}));
    EXPECT_EQ(document.style(at("B2")), right_two_digits);
    EXPECT_FALSE(document.used_range().has_value());
}

TEST_F(DocumentEditTest, ClearingInputKeepsStyle) {
    document.apply(edit_to({{"A1", input("5", right_two_digits)}}));
    document.set_input(at("A1"), "");
    EXPECT_EQ(document.style(at("A1")), right_two_digits);
}

TEST_F(DocumentEditTest, InverseRestoresValuesAndStyles) {
    document.set_input(at("A1"), "1");
    document.set_input(at("A3"), "=A1+A2");
    const DocumentEdit edit =
        edit_to({{"A1", input("10", right_two_digits)}, {"A2", input("5")}});
    document.apply(edit);
    EXPECT_EQ(shown("A3"), "15");

    document.apply(edit.inverse());
    EXPECT_EQ(document.raw(at("A1")), "1");
    EXPECT_EQ(document.raw(at("A2")), "");
    EXPECT_EQ(document.style(at("A1")), CellStyle{});
    EXPECT_EQ(shown("A3"), "1");
}

TEST_F(DocumentEditTest, InverseGoesBackwards) {
    const DocumentEdit edit = edit_to({{"A1", input("1")}, {"A2", input("2")}});
    const DocumentEdit inverse = edit.inverse();
    ASSERT_EQ(inverse.cells.size(), 2U);
    EXPECT_EQ(inverse.cells[0].address, at("A2"));
    EXPECT_EQ(inverse.cells[0].after.raw, "");
    EXPECT_EQ(inverse.cells[0].before.raw, "2");
}

TEST_F(DocumentEditTest, ParseErrorLeavesDocumentUntouched) {
    document.set_input(at("A1"), "1");
    CountingObserver observer;
    document.subscribe(&observer);
    EXPECT_THROW(document.apply(edit_to({{"A1", input("2", right_two_digits)},
                                         {"A2", input("=1+")}})),
                 ss::core::ParseError);
    EXPECT_EQ(document.raw(at("A1")), "1");
    EXPECT_EQ(document.style(at("A1")), CellStyle{});
    EXPECT_EQ(document.raw(at("A2")), "");
    EXPECT_EQ(observer.calls, 0);
    document.unsubscribe(&observer);
}

TEST_F(DocumentEditTest, AddressOutsideSheetLeavesDocumentUntouched) {
    DocumentEdit edit = edit_to({{"A1", input("2")}});
    edit.cells.push_back(
        {.address = {.row = ss::core::max_rows, .col = 0}, .before = {}, .after = input("1")});
    EXPECT_THROW(document.apply(edit), ss::core::LimitError);
    EXPECT_EQ(document.raw(at("A1")), "");
}

TEST_F(DocumentEditTest, FailureInTheMiddleIsRolledBack) {
    document.set_input(at("A1"), "1");
    document.set_input(at("B1"), "=A1*2");
    fail = true;
    EXPECT_THROW(document.apply(edit_to({{"A1", input("7", right_two_digits)},
                                         {"A2", input("=FAIL()")}})),
                 std::runtime_error);
    fail = false;
    EXPECT_EQ(document.raw(at("A1")), "1");
    EXPECT_EQ(document.style(at("A1")), CellStyle{});
    EXPECT_EQ(document.raw(at("A2")), "");
    EXPECT_EQ(shown("B1"), "2");
}

TEST_F(DocumentEditTest, WidthsAreAppliedAndInverted) {
    ColumnWidths wide;
    wide.set(1, 25);
    DocumentEdit edit = edit_to({{"A1", input("1")}});
    edit.widths_before = document.column_widths();
    edit.widths_after = wide;

    document.apply(edit);
    EXPECT_EQ(document.column_widths().get(1), 25);
    EXPECT_EQ(document.raw(at("A1")), "1");

    document.apply(edit.inverse());
    EXPECT_EQ(document.column_widths(), ColumnWidths{});
    EXPECT_EQ(document.raw(at("A1")), "");
}

// правка без ширин их не сбрасывает
TEST_F(DocumentEditTest, EditWithoutWidthsKeepsThem) {
    ColumnWidths wide;
    wide.set(0, 30);
    DocumentEdit widen;
    widen.widths_before = document.column_widths();
    widen.widths_after = wide;
    document.apply(widen);

    document.apply(edit_to({{"A1", input("1")}}));
    EXPECT_EQ(document.column_widths().get(0), 30);
}

TEST_F(DocumentEditTest, WidthsAreRolledBackOnFailure) {
    ColumnWidths wide;
    wide.set(0, 30);
    DocumentEdit edit = edit_to({{"A1", input("=FAIL()")}});
    edit.widths_before = document.column_widths();
    edit.widths_after = wide;
    fail = true;
    EXPECT_THROW(document.apply(edit), std::runtime_error);
    fail = false;
    EXPECT_EQ(document.column_widths(), ColumnWidths{});
    EXPECT_EQ(document.raw(at("A1")), "");
}

TEST_F(DocumentEditTest, WidthsAreUntouchedOnParseError) {
    ColumnWidths wide;
    wide.set(0, 30);
    DocumentEdit edit = edit_to({{"A1", input("=1+")}});
    edit.widths_before = document.column_widths();
    edit.widths_after = wide;
    EXPECT_THROW(document.apply(edit), ss::core::ParseError);
    EXPECT_EQ(document.column_widths(), ColumnWidths{});
}

TEST_F(DocumentEditTest, CycleInsideOneEdit) {
    document.apply(edit_to({{"A1", input("=A2")}, {"A2", input("=A1")}}));
    EXPECT_EQ(shown("A1"), "#CYCLE!");
    EXPECT_EQ(shown("A2"), "#CYCLE!");
}

TEST_F(DocumentEditTest, ContentOfEmptyCell) {
    EXPECT_EQ(document.content(at("C3")), CellContent{});
}
