#include <optional>

#include <gtest/gtest.h>

#include "core/value.hpp"
#include "support/formula_fixture.hpp"

using ss::core::ErrorCode;
using ss::core::make_error;
using ss::core::make_number;
using ss::core::make_string;

namespace {

class Evaluator : public ss::test::FormulaTest {};

}  // namespace

TEST_F(Evaluator, ComputesLiterals) {
    EXPECT_EQ(number("=12"), 12);
    EXPECT_EQ(eval(R"(="abc")"), "abc");
    EXPECT_EQ(number(R"(="12")"), std::nullopt);  // строка, а не число
}

TEST_F(Evaluator, ComputesArithmetic) {
    EXPECT_EQ(number("=1-2-3"), -4);
    EXPECT_EQ(number("=2*3+4"), 10);
    EXPECT_EQ(number("=2+3*4"), 14);
    EXPECT_EQ(number("=(2+3)*4"), 20);
    EXPECT_EQ(number("=7/2"), 3.5);
    EXPECT_EQ(number("=--1"), 1);
    EXPECT_EQ(number("=+5"), 5);
    EXPECT_EQ(eval("=0.1+0.2"), "0.3");
}

TEST_F(Evaluator, ReadsCellValues) {
    sheet.set("A1", make_number(3));
    sheet.set("B2", make_string("text"));
    EXPECT_EQ(number("=A1+1"), 4);
    EXPECT_EQ(number("=-A1*2"), -6);
    EXPECT_EQ(eval("=B2"), "text");
}

TEST_F(Evaluator, EmptyCellIsZero) {
    EXPECT_EQ(number("=C5+1"), 1);
    EXPECT_EQ(number("=C5*2"), 0);
    // сама формула =C5 тоже показывает 0, а не пустоту
    EXPECT_EQ(eval("=C5"), "0");
    EXPECT_EQ(number("=C5"), 0);
}

TEST_F(Evaluator, DivisionByZero) {
    EXPECT_EQ(eval("=1/0"), "#DIV/0!");
    EXPECT_EQ(eval("=1/C5"), "#DIV/0!");
    EXPECT_EQ(eval("=0/0"), "#DIV/0!");
}

TEST_F(Evaluator, ReferenceOutsideSheet) {
    EXPECT_EQ(eval("=A99999999"), "#REF!");
    EXPECT_EQ(eval("=AAA1+1"), "#REF!");
    EXPECT_EQ(eval("=SUM(A1:A99999999)"), "#REF!");
}

TEST_F(Evaluator, StringInArithmeticIsValueError) {
    sheet.set("A1", make_string("12"));
    EXPECT_EQ(eval(R"(="a"+1)"), "#VALUE!");
    EXPECT_EQ(eval("=A1*2"), "#VALUE!");
    EXPECT_EQ(eval(R"(=-"a")"), "#VALUE!");
}

TEST_F(Evaluator, OverflowIsValueError) {
    EXPECT_EQ(eval("=1e308*10"), "#VALUE!");
    EXPECT_EQ(eval("=-1e308-1e308"), "#VALUE!");
}

TEST_F(Evaluator, PropagatesErrors) {
    sheet.set("A1", make_error(ErrorCode::Cycle));
    EXPECT_EQ(eval("=A1+1"), "#CYCLE!");
    EXPECT_EQ(eval("=-A1"), "#CYCLE!");
    EXPECT_EQ(eval("=A1<1"), "#CYCLE!");
    EXPECT_EQ(eval("=-(1/0)"), "#DIV/0!");
}

TEST_F(Evaluator, LeftErrorWins) {
    EXPECT_EQ(eval("=1/0+A99999999"), "#DIV/0!");
    EXPECT_EQ(eval("=A99999999+1/0"), "#REF!");
    EXPECT_EQ(eval(R"(="a"+1/0)"), "#DIV/0!");  // ошибка операнда важнее #VALUE!
}

TEST_F(Evaluator, ComparesNumbers) {
    EXPECT_EQ(number("=1<2"), 1);
    EXPECT_EQ(number("=2<1"), 0);
    EXPECT_EQ(number("=2<=2"), 1);
    EXPECT_EQ(number("=3>2"), 1);
    EXPECT_EQ(number("=2>=3"), 0);
    EXPECT_EQ(number("=2=2"), 1);
    EXPECT_EQ(number("=2<>2"), 0);
    EXPECT_EQ(number("=1+1=2"), 1);
}

TEST_F(Evaluator, ComparesStringsIgnoringCase) {
    EXPECT_EQ(number(R"(="abc"="ABC")"), 1);
    EXPECT_EQ(number(R"(="abc"<"abd")"), 1);
    EXPECT_EQ(number(R"(="b">"A")"), 1);
}

TEST_F(Evaluator, NumberAndStringAreIncomparable) {
    EXPECT_EQ(eval(R"(=1="1")"), "#VALUE!");
    EXPECT_EQ(eval(R"(="a"<1)"), "#VALUE!");
}

TEST_F(Evaluator, EmptyCellComparesAsZeroOrBlank) {
    EXPECT_EQ(number("=C5=0"), 1);
    EXPECT_EQ(number(R"(=C5="")"), 1);
    EXPECT_EQ(number("=C5=D7"), 1);
    EXPECT_EQ(number("=C5<1"), 1);
}

TEST_F(Evaluator, RangeOutsideFunctionIsValueError) {
    EXPECT_EQ(eval("=A1:B2"), "#VALUE!");
    EXPECT_EQ(eval("=A1:B2+1"), "#VALUE!");
}

TEST_F(Evaluator, UnknownFunctionIsNameError) {
    EXPECT_EQ(eval("=NOSUCH(1)"), "#NAME?");
    EXPECT_EQ(eval("=NOSUCH(1/0)"), "#NAME?");
    EXPECT_EQ(eval("=1+NOSUCH()"), "#NAME?");
}
