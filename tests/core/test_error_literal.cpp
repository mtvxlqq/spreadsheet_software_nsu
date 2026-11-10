#include <string>

#include <gtest/gtest.h>

#include "core/errors.hpp"
#include "core/formula_printer.hpp"
#include "core/parser.hpp"
#include "core/ref_collector.hpp"
#include "support/formula_fixture.hpp"

using ss::core::FormulaPrinter;
using ss::core::ParseError;
using ss::core::Parser;
using ss::core::RefCollector;

namespace {

class ErrorLiteralTest : public ss::test::FormulaTest {
protected:
    std::string reprinted(std::string_view formula) {
        return FormulaPrinter::print(*Parser::parse(formula, functions));
    }
};

}  // namespace

TEST_F(ErrorLiteralTest, EvaluatesToRefError) {
    EXPECT_EQ(eval("=#REF!"), "#REF!");
}

TEST_F(ErrorLiteralTest, SpreadsThroughArithmetic) {
    EXPECT_EQ(eval("=#REF!+1"), "#REF!");
    EXPECT_EQ(eval("=1+#REF!"), "#REF!");
    EXPECT_EQ(eval("=-#REF!"), "#REF!");
}

TEST_F(ErrorLiteralTest, SpreadsThroughFunctions) {
    EXPECT_EQ(eval("=SUM(#REF!,1)"), "#REF!");
}

TEST_F(ErrorLiteralTest, LowerCaseIsAccepted) {
    EXPECT_EQ(eval("=#ref!"), "#REF!");
}

TEST_F(ErrorLiteralTest, PrintsAndParsesBack) {
    EXPECT_EQ(reprinted("=#REF!"), "=#REF!");
    EXPECT_EQ(reprinted(reprinted("=A1+#REF!*2")), "=A1+#REF!*2");
}

TEST_F(ErrorLiteralTest, HasNoReferences) {
    EXPECT_TRUE(RefCollector::collect(*Parser::parse("=#REF!", functions)).empty());
}

TEST_F(ErrorLiteralTest, OtherErrorCodesAreNotFormulas) {
    EXPECT_THROW(Parser::parse("=#DIV/0!", functions), ParseError);
    EXPECT_THROW(Parser::parse("=#NAME?", functions), ParseError);
    EXPECT_THROW(Parser::parse("=#", functions), ParseError);
    EXPECT_THROW(Parser::parse("=#REF", functions), ParseError);
}

TEST_F(ErrorLiteralTest, CloneKeepsCode) {
    const auto formula = Parser::parse("=#REF!", functions);
    EXPECT_EQ(FormulaPrinter::print(*formula->clone()), "=#REF!");
}
