#include <string>

#include <gtest/gtest.h>

#include "core/expr.hpp"
#include "core/formula_printer.hpp"
#include "support/expr_builders.hpp"

using ss::core::BinaryOp;
using ss::core::ExprPtr;
using ss::core::FormulaPrinter;
using namespace ss::test;

namespace {

std::string print(const ExprPtr& expr) {
    return FormulaPrinter::print(*expr);
}

}  // namespace

TEST(FormulaPrinter, PrintsNumbersExactly) {
    EXPECT_EQ(print(num(3)), "=3");
    EXPECT_EQ(print(num(2.5)), "=2.5");
    EXPECT_EQ(print(num(0.1)), "=0.1");
    EXPECT_EQ(print(num(1e20)), "=1e+20");
    // в формуле число должно сохраниться точно, 10 знаков как в ячейке тут мало
    EXPECT_EQ(print(num(0.1 + 0.2)), "=0.30000000000000004");
}

TEST(FormulaPrinter, QuotesStrings) {
    EXPECT_EQ(print(str("abc")), R"(="abc")");
    EXPECT_EQ(print(str(R"(a"b)")), R"(="a""b")");
    EXPECT_EQ(print(str("")), R"(="")");
}

TEST(FormulaPrinter, PrintsReferencesInUpperCase) {
    EXPECT_EQ(print(cell("b2")), "=B2");
    EXPECT_EQ(print(cell("$a$1")), "=$A$1");
    EXPECT_EQ(print(range_of("a1", "B$3")), "=A1:B$3");
}

TEST(FormulaPrinter, AddsParenthesesOnlyWhenNeeded) {
    using enum BinaryOp;
    EXPECT_EQ(print(bin(Mul, bin(Add, num(1), num(2)), num(3))), "=(1+2)*3");
    EXPECT_EQ(print(bin(Add, num(1), bin(Mul, num(2), num(3)))), "=1+2*3");
    EXPECT_EQ(print(bin(Add, bin(Mul, num(1), num(2)), num(3))), "=1*2+3");
    EXPECT_EQ(print(bin(Mul, num(1), bin(Sub, num(2), num(3)))), "=1*(2-3)");
}

TEST(FormulaPrinter, KeepsTreeShapeForSameOperators) {
    using enum BinaryOp;
    EXPECT_EQ(print(bin(Sub, bin(Sub, num(1), num(2)), num(3))), "=1-2-3");
    EXPECT_EQ(print(bin(Sub, num(1), bin(Sub, num(2), num(3)))), "=1-(2-3)");
    EXPECT_EQ(print(bin(Div, bin(Div, num(8), num(4)), num(2))), "=8/4/2");
    EXPECT_EQ(print(bin(Div, num(8), bin(Div, num(4), num(2)))), "=8/(4/2)");
    // для сложения результат тот же, но дерево другое, его и сохраняем
    EXPECT_EQ(print(bin(Add, num(1), bin(Add, num(2), num(3)))), "=1+(2+3)");
}

TEST(FormulaPrinter, PrintsUnaryOperators) {
    using enum BinaryOp;
    EXPECT_EQ(print(neg(num(1))), "=-1");
    EXPECT_EQ(print(neg(neg(num(1)))), "=--1");
    EXPECT_EQ(print(unary_plus(cell("A1"))), "=+A1");
    EXPECT_EQ(print(bin(Mul, neg(cell("A1")), num(2))), "=-A1*2");
    EXPECT_EQ(print(neg(bin(Mul, cell("A1"), num(2)))), "=-(A1*2)");
    EXPECT_EQ(print(neg(bin(Add, num(1), num(2)))), "=-(1+2)");
    EXPECT_EQ(print(bin(Sub, num(1), neg(num(2)))), "=1--2");
}

TEST(FormulaPrinter, PrintsComparisons) {
    using enum BinaryOp;
    EXPECT_EQ(print(bin(Eq, cell("A1"), num(1))), "=A1=1");
    EXPECT_EQ(print(bin(Ne, cell("A1"), num(1))), "=A1<>1");
    EXPECT_EQ(print(bin(Lt, cell("A1"), num(1))), "=A1<1");
    EXPECT_EQ(print(bin(Le, cell("A1"), num(1))), "=A1<=1");
    EXPECT_EQ(print(bin(Gt, cell("A1"), num(1))), "=A1>1");
    EXPECT_EQ(print(bin(Ge, cell("A1"), num(1))), "=A1>=1");
    EXPECT_EQ(print(bin(Eq, bin(Add, cell("A1"), num(1)), num(2))), "=A1+1=2");
    EXPECT_EQ(print(bin(Add, bin(Lt, num(1), num(2)), num(1))), "=(1<2)+1");
}

TEST(FormulaPrinter, ParenthesizesNestedComparisons) {
    using enum BinaryOp;
    // без скобок вышло бы 1=2=0, а такое парсер не принимает
    EXPECT_EQ(print(bin(Eq, bin(Eq, num(1), num(2)), num(0))), "=(1=2)=0");
    EXPECT_EQ(print(bin(Eq, num(0), bin(Eq, num(1), num(2)))), "=0=(1=2)");
}

TEST(FormulaPrinter, PrintsFunctionCalls) {
    using enum BinaryOp;
    EXPECT_EQ(print(call("pi")), "=PI()");
    EXPECT_EQ(print(call("sum", range_of("A1", "B2"))), "=SUM(A1:B2)");
    EXPECT_EQ(print(call("If", bin(Gt, cell("A1"), num(0)), num(1), neg(num(1)))),
              "=IF(A1>0,1,-1)");
    EXPECT_EQ(print(call("SUM", cell("A1"), call("max", cell("B1"), num(2)))),
              "=SUM(A1,MAX(B1,2))");
    EXPECT_EQ(print(bin(Mul, call("SUM", bin(Add, num(1), num(2))), num(3))), "=SUM(1+2)*3");
}

TEST(ExprClone, MakesIndependentCopy) {
    using enum BinaryOp;
    ExprPtr original =
        bin(Add, call("SUM", range_of("A1", "A3"), str("x"), num(2)), neg(cell("$B$2")));
    const std::string text = print(original);
    ExprPtr copy = original->clone();
    EXPECT_NE(copy.get(), original.get());

    // если копия где-то держит узлы оригинала, ASan поймает обращение к удалённой памяти
    original.reset();
    EXPECT_EQ(print(copy), text);
    EXPECT_EQ(text, R"(=SUM(A1:A3,"x",2)+-$B$2)");
}
