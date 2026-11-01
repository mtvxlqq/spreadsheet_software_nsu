#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include <gtest/gtest.h>

#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/eval_context.hpp"
#include "core/function.hpp"
#include "core/function_registry.hpp"
#include "core/value.hpp"
#include "support/formula_fixture.hpp"

using ss::core::ErrorCode;
using ss::core::EvalContext;
using ss::core::Expr;
using ss::core::Function;
using ss::core::FunctionRegistry;
using ss::core::make_error;
using ss::core::make_number;
using ss::core::make_string;
using ss::core::ValuePtr;

namespace {

// A1..A4 = 1, 2, "x", пусто
class Functions : public ss::test::FormulaTest {
protected:
    Functions() {
        sheet.set("A1", make_number(1));
        sheet.set("A2", make_number(2));
        sheet.set("A3", make_string("x"));
    }
};

// Функция, которой нет среди встроенных: проверяем, что новую функцию можно
// добавить, не трогая парсер, вычислитель и реестр.
class TwiceFunction final : public Function {
public:
    std::string_view name() const override {
        return "TWICE";
    }

    ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const override {
        if (args.size() != 1) {
            return make_error(ErrorCode::Value);
        }
        const std::optional<double> number = context.evaluate(*args[0])->as_number();
        return number ? make_number(*number * 2) : make_error(ErrorCode::Value);
    }
};

}  // namespace

TEST_F(Functions, SumAddsNumbers) {
    EXPECT_EQ(number("=SUM(1,2,3)"), 6);
    EXPECT_EQ(number("=SUM(5)"), 5);
    EXPECT_EQ(number("=SUM(A1:A4)"), 3);  // строку и пустую пропускаем
    EXPECT_EQ(number("=SUM(A1:A4, 10, A1)"), 14);
    EXPECT_EQ(number("=SUM(B1:B9)"), 0);
    EXPECT_EQ(number("=SUM(A4)"), 0);
}

TEST_F(Functions, StringArgumentIsValueError) {
    EXPECT_EQ(eval(R"(=SUM("x"))"), "#VALUE!");
    EXPECT_EQ(eval("=SUM(A3)"), "#VALUE!");
    EXPECT_EQ(eval(R"(=MAX(1, "2"))"), "#VALUE!");
    // а из диапазона, даже из одной ячейки, строка просто пропускается
    EXPECT_EQ(number("=SUM(A3:A3)"), 0);
}

TEST_F(Functions, ReturnFirstError) {
    sheet.set("B1", make_error(ErrorCode::Div0));
    sheet.set("B2", make_error(ErrorCode::Ref));
    EXPECT_EQ(eval("=SUM(B1:B2)"), "#DIV/0!");
    EXPECT_EQ(eval("=SUM(B2:B1)"), "#DIV/0!");  // диапазон всё равно идёт сверху вниз
    EXPECT_EQ(eval("=SUM(1, 1/0, A99999999)"), "#DIV/0!");
    EXPECT_EQ(eval("=MIN(A1:A4, B2)"), "#REF!");
    EXPECT_EQ(eval("=AVERAGE(B1)"), "#DIV/0!");
}

TEST_F(Functions, NoArgumentsIsValueError) {
    EXPECT_EQ(eval("=SUM()"), "#VALUE!");
    EXPECT_EQ(eval("=MIN()"), "#VALUE!");
    EXPECT_EQ(eval("=MAX()"), "#VALUE!");
    EXPECT_EQ(eval("=AVERAGE()"), "#VALUE!");
}

TEST_F(Functions, SumOverflowIsValueError) {
    EXPECT_EQ(eval("=SUM(1e308, 1e308)"), "#VALUE!");
}

TEST_F(Functions, MinAndMax) {
    EXPECT_EQ(number("=MIN(3,1,2)"), 1);
    EXPECT_EQ(number("=MAX(3,1,2)"), 3);
    EXPECT_EQ(number("=MAX(-5,-2)"), -2);
    EXPECT_EQ(number("=MIN(A1:A4)"), 1);
    EXPECT_EQ(number("=MAX(A1:A4, -7)"), 2);
}

TEST_F(Functions, MinAndMaxWithoutNumbersGiveZero) {
    EXPECT_EQ(number("=MIN(B1:B9)"), 0);
    EXPECT_EQ(number("=MAX(A3:A4)"), 0);
}

TEST_F(Functions, Average) {
    EXPECT_EQ(number("=AVERAGE(1,2,3,4)"), 2.5);
    EXPECT_EQ(number("=AVERAGE(A1:A4)"), 1.5);  // делим на 2, а не на 4
    EXPECT_EQ(eval("=AVERAGE(B1:B9)"), "#DIV/0!");
    EXPECT_EQ(eval("=AVERAGE(A3:A4)"), "#DIV/0!");
}

TEST_F(Functions, IfPicksBranch) {
    EXPECT_EQ(eval(R"(=IF(1, "yes", "no"))"), "yes");
    EXPECT_EQ(eval(R"(=IF(0, "yes", "no"))"), "no");
    EXPECT_EQ(eval(R"(=IF(-0.5, "yes", "no"))"), "yes");
    EXPECT_EQ(number("=IF(A1>1, 10, 20)"), 20);
    EXPECT_EQ(number("=IF(A2>1, 10, 20)"), 10);
}

TEST_F(Functions, IfWithoutElseGivesZero) {
    EXPECT_EQ(number("=IF(0, 5)"), 0);
    EXPECT_EQ(number("=IF(1, 5)"), 5);
}

TEST_F(Functions, IfEvaluatesOnlyChosenBranch) {
    EXPECT_EQ(number("=IF(1, 1, 1/0)"), 1);
    EXPECT_EQ(number("=IF(0, 1/0, 2)"), 2);
    EXPECT_EQ(number("=IF(0, NOSUCH(), 2)"), 2);
    EXPECT_EQ(eval("=IF(1, 1/0, 2)"), "#DIV/0!");
}

TEST_F(Functions, IfCondition) {
    EXPECT_EQ(eval("=IF(1/0, 1, 2)"), "#DIV/0!");
    EXPECT_EQ(eval(R"(=IF("x", 1, 2))"), "#VALUE!");
    EXPECT_EQ(eval("=IF(A3, 1, 2)"), "#VALUE!");
    EXPECT_EQ(number("=IF(A4, 1, 2)"), 2);  // пустая ячейка это ложь
}

TEST_F(Functions, IfNeedsTwoOrThreeArguments) {
    EXPECT_EQ(eval("=IF()"), "#VALUE!");
    EXPECT_EQ(eval("=IF(1)"), "#VALUE!");
    EXPECT_EQ(eval("=IF(1, 2, 3, 4)"), "#VALUE!");
}

TEST_F(Functions, IfBranchFollowsUsualRules) {
    EXPECT_EQ(eval("=IF(1, A4)"), "0");
    EXPECT_EQ(eval("=IF(1, A1:A2)"), "#VALUE!");
}

TEST_F(Functions, NamesIgnoreCase) {
    EXPECT_EQ(number("=sum(1,2)"), 3);
    EXPECT_EQ(number("=Average(2,4)"), 3);
    EXPECT_EQ(number("=if(1, max(1,5), 0)"), 5);
}

TEST_F(Functions, NestedCalls) {
    EXPECT_EQ(number("=SUM(1, MAX(A1:A4), IF(A1, 10))"), 13);
    EXPECT_EQ(number("=AVERAGE(MIN(4,6), MAX(4,6)) * 2"), 10);
}

TEST_F(Functions, NewFunctionNeedsOnlyRegistration) {
    functions.add("TWICE", [] { return std::make_shared<TwiceFunction>(); });
    EXPECT_EQ(number("=TWICE(A2) + 1"), 5);
    EXPECT_EQ(number("=SUM(TWICE(3), 1)"), 7);
}

TEST(FunctionRegistry, CreatesByNameIgnoringCase) {
    FunctionRegistry registry;
    ss::core::register_builtins(registry);
    for (const std::string_view name : {"SUM", "min", "Max", "average", "IF"}) {
        ASSERT_NE(registry.create(name), nullptr) << name;
    }
    EXPECT_EQ(registry.create("if")->name(), "IF");
    EXPECT_EQ(registry.create("NOSUCH"), nullptr);
    EXPECT_EQ(registry.create(""), nullptr);
}

TEST(FunctionRegistry, RejectsDuplicateNames) {
    FunctionRegistry registry;
    registry.add("TWICE", [] { return std::make_shared<TwiceFunction>(); });
    EXPECT_THROW(registry.add("TWICE", [] { return std::make_shared<TwiceFunction>(); }),
                 ss::core::Error);
    EXPECT_THROW(registry.add("twice", [] { return std::make_shared<TwiceFunction>(); }),
                 ss::core::Error);
}

TEST(FunctionRegistry, BuiltinsCannotBeRegisteredTwice) {
    FunctionRegistry registry;
    ss::core::register_builtins(registry);
    EXPECT_THROW(ss::core::register_builtins(registry), ss::core::Error);
}
