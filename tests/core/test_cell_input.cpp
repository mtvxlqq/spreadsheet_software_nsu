#include <optional>
#include <string_view>
#include <utility>

#include <gtest/gtest.h>

#include "core/builtins.hpp"
#include "core/cell.hpp"
#include "core/cell_input.hpp"
#include "core/errors.hpp"
#include "core/formula_printer.hpp"
#include "core/function_registry.hpp"

using ss::core::Cell;
using ss::core::CellInputParser;

namespace {

class CellInput : public ::testing::Test {
protected:
    CellInput() {
        ss::core::register_builtins(functions);
    }

    std::optional<Cell> parse(std::string_view input) const {
        return parser.parse(input);
    }

    ss::core::FunctionRegistry functions;
    CellInputParser parser{functions};
};

}  // namespace

TEST_F(CellInput, EmptyInputClearsCell) {
    EXPECT_FALSE(parse("").has_value());
    EXPECT_FALSE(parse("   ").has_value());
    EXPECT_FALSE(parse("\t").has_value());
}

TEST_F(CellInput, ReadsNumbers) {
    const std::pair<std::string_view, double> cases[] = {
        {"12", 12.0}, {"-3.5", -3.5}, {".5", 0.5}, {"1e3", 1000.0}, {"0", 0.0}};
    for (const auto& [input, expected] : cases) {
        const std::optional<Cell> cell = parse(input);
        ASSERT_TRUE(cell.has_value()) << input;
        EXPECT_EQ(cell->value->as_number(), expected) << input;
        EXPECT_EQ(cell->formula, nullptr);
        EXPECT_EQ(cell->raw, input);
    }
}

// решили так: пробелы по краям обрезаются, и в raw тоже
TEST_F(CellInput, TrimsSpaces) {
    const std::optional<Cell> number = parse(" 12 ");
    ASSERT_TRUE(number.has_value());
    EXPECT_EQ(number->value->as_number(), 12);
    EXPECT_EQ(number->raw, "12");

    const std::optional<Cell> text = parse("  hello world ");
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(text->value->to_text(), "hello world");
}

TEST_F(CellInput, NotQuiteNumberIsText) {
    for (const std::string_view input : {"12abc", "1 2", "inf", "nan", "1e999", "0x10", "+5"}) {
        const std::optional<Cell> cell = parse(input);
        ASSERT_TRUE(cell.has_value()) << input;
        EXPECT_EQ(cell->value->as_number(), std::nullopt) << input;
        EXPECT_EQ(cell->value->to_text(), input);
    }
}

TEST_F(CellInput, ApostropheMakesText) {
    const std::optional<Cell> cell = parse("'12");
    ASSERT_TRUE(cell.has_value());
    EXPECT_EQ(cell->value->as_number(), std::nullopt);
    EXPECT_EQ(cell->value->to_text(), "12");
    EXPECT_EQ(cell->raw, "'12");

    EXPECT_EQ(parse("'=1+1")->value->to_text(), "=1+1");
    EXPECT_EQ(parse("'")->value->to_text(), "");
}

TEST_F(CellInput, ReadsFormula) {
    const std::optional<Cell> cell = parse("=a1 + 1");
    ASSERT_TRUE(cell.has_value());
    ASSERT_NE(cell->formula, nullptr);
    EXPECT_EQ(ss::core::FormulaPrinter::print(*cell->formula), "=A1+1");
    EXPECT_EQ(cell->raw, "=a1 + 1");
    // посчитает потом Recalculator
    EXPECT_TRUE(cell->value->is_empty());
}

TEST_F(CellInput, BrokenFormulaThrows) {
    EXPECT_THROW(parse("=1+"), ss::core::ParseError);
    EXPECT_THROW(parse("  =(1"), ss::core::ParseError);
}
