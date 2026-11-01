#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/errors.hpp"
#include "core/expr.hpp"
#include "core/expr_visitor.hpp"
#include "core/formula_printer.hpp"
#include "core/function.hpp"
#include "core/parser.hpp"
#include "core/text.hpp"
#include "core/value.hpp"

using ss::core::BinaryExpr;
using ss::core::BinaryOp;
using ss::core::EvalContext;
using ss::core::Expr;
using ss::core::ExprPtr;
using ss::core::FormulaPrinter;
using ss::core::Function;
using ss::core::FunctionCallExpr;
using ss::core::FunctionProvider;
using ss::core::NumberLit;
using ss::core::ParseError;
using ss::core::Parser;
using ss::core::RangeExpr;
using ss::core::RefExpr;
using ss::core::StringLit;
using ss::core::UnaryExpr;
using ss::core::UnaryOp;
using ss::core::ValuePtr;

namespace {

// Функция-пустышка: парсеру важно только, что она нашлась.
class FakeFunction final : public Function {
public:
    explicit FakeFunction(std::string name) : name_(std::move(name)) {}

    std::string_view name() const override {
        return name_;
    }

    ValuePtr call(std::span<const Expr* const> /*args*/, EvalContext& /*context*/) const override {
        return ss::core::empty_value();
    }

private:
    std::string name_;
};

// Знает только SUM, MAX, IF и PI. Парсеру этого хватает, настоящий реестр
// ему не нужен.
class FakeFunctions final : public FunctionProvider {
public:
    std::shared_ptr<const Function> create(std::string_view name) const override {
        const std::string upper = ss::core::to_upper_ascii(name);
        if (upper == "SUM" || upper == "MAX" || upper == "IF" || upper == "PI") {
            return std::make_shared<FakeFunction>(upper);
        }
        return nullptr;
    }
};

// Печатает дерево, беря в скобки каждую операцию, чтобы в тестах была видна
// его форма: =1-2-3 даёт ((1-2)-3).
class ShapePrinter final : public ss::core::ExprVisitor {
public:
    static std::string print(const Expr& expr) {
        ShapePrinter printer;
        expr.accept(printer);
        return printer.text_;
    }

    void visit(const NumberLit& node) override {
        text_ = std::format("{}", node.value());
    }

    void visit(const StringLit& node) override {
        text_ = "\"" + node.text() + "\"";
    }

    void visit(const RefExpr& node) override {
        text_ = node.ref().to_string();
    }

    void visit(const RangeExpr& node) override {
        text_ = node.from().to_string() + ":" + node.to().to_string();
    }

    void visit(const UnaryExpr& node) override {
        text_ = std::string("(") + (node.op() == UnaryOp::Neg ? "-" : "+") +
                print(node.operand()) + ")";
    }

    void visit(const BinaryExpr& node) override {
        text_ = "(" + print(node.left()) + op_text(node.op()) + print(node.right()) + ")";
    }

    void visit(const FunctionCallExpr& node) override {
        text_ = ss::core::to_upper_ascii(node.name()) + "(";
        for (std::size_t i = 0; i < node.args().size(); ++i) {
            text_ += (i > 0 ? "," : "") + print(*node.args()[i]);
        }
        text_ += ")";
    }

private:
    ShapePrinter() = default;

    static std::string op_text(BinaryOp op) {
        switch (op) {
            case BinaryOp::Add:
                return "+";
            case BinaryOp::Sub:
                return "-";
            case BinaryOp::Mul:
                return "*";
            case BinaryOp::Div:
                return "/";
            case BinaryOp::Eq:
                return "=";
            case BinaryOp::Ne:
                return "<>";
            case BinaryOp::Lt:
                return "<";
            case BinaryOp::Le:
                return "<=";
            case BinaryOp::Gt:
                return ">";
            case BinaryOp::Ge:
                return ">=";
        }
        return "?";
    }

    std::string text_;
};

const FakeFunctions functions{};

ExprPtr parse(std::string_view formula) {
    return Parser::parse(formula, functions);
}

std::string shape(std::string_view formula) {
    return ShapePrinter::print(*parse(formula));
}

// корень дерева должен оказаться вызовом функции
const FunctionCallExpr& as_call(const ExprPtr& expr) {
    // Посетитель вместо dynamic_cast, как и в основном коде.
    class CallFinder final : public ss::core::ExprVisitor {
    public:
        const FunctionCallExpr* found = nullptr;
        void visit(const NumberLit& /*node*/) override {}
        void visit(const StringLit& /*node*/) override {}
        void visit(const RefExpr& /*node*/) override {}
        void visit(const RangeExpr& /*node*/) override {}
        void visit(const UnaryExpr& /*node*/) override {}
        void visit(const BinaryExpr& /*node*/) override {}
        void visit(const FunctionCallExpr& node) override {
            found = &node;
        }
    };
    CallFinder finder;
    expr->accept(finder);
    if (finder.found == nullptr) {
        throw std::logic_error("в корне не вызов функции");
    }
    return *finder.found;
}

void expect_error_at(std::string_view formula, std::size_t position) {
    try {
        parse(formula);
        ADD_FAILURE() << "нет ParseError для: " << formula;
    } catch (const ParseError& error) {
        EXPECT_EQ(error.position(), position) << formula << ": " << error.what();
    }
}

}  // namespace

TEST(Parser, ReadsLiterals) {
    EXPECT_EQ(shape("=12"), "12");
    EXPECT_EQ(shape("=.5"), "0.5");
    EXPECT_EQ(shape("=1e3"), "1000");
    EXPECT_EQ(shape("=1.5E-3"), "0.0015");
    EXPECT_EQ(shape(R"(="a""b")"), R"("a"b")");
    EXPECT_EQ(shape("=a1"), "A1");
    EXPECT_EQ(shape("=$b$2"), "$B$2");
    EXPECT_EQ(shape("= 1 "), "1");
}

TEST(Parser, MultiplicationBindsStronger) {
    EXPECT_EQ(shape("=2*3+4"), "((2*3)+4)");
    EXPECT_EQ(shape("=2+3*4"), "(2+(3*4))");
    EXPECT_EQ(shape("=1+2/4-3"), "((1+(2/4))-3)");
}

TEST(Parser, SameLevelIsLeftAssociative) {
    EXPECT_EQ(shape("=1-2-3"), "((1-2)-3)");
    EXPECT_EQ(shape("=8/4/2"), "((8/4)/2)");
    EXPECT_EQ(shape("=1+2-3+4"), "(((1+2)-3)+4)");
}

TEST(Parser, ReadsUnaryOperators) {
    EXPECT_EQ(shape("=-1"), "(-1)");
    EXPECT_EQ(shape("=--1"), "(-(-1))");
    EXPECT_EQ(shape("=+-1"), "(+(-1))");
    EXPECT_EQ(shape("=-A1*2"), "((-A1)*2)");
    EXPECT_EQ(shape("=2*-3"), "(2*(-3))");
    EXPECT_EQ(shape("=1--1"), "(1-(-1))");
}

TEST(Parser, ParenthesesChangeOrder) {
    EXPECT_EQ(shape("=(1+2)*3"), "((1+2)*3)");
    EXPECT_EQ(shape("=1-(2-3)"), "(1-(2-3))");
    EXPECT_EQ(shape("=((1))"), "1");
    EXPECT_EQ(shape("=-(1+2)"), "(-(1+2))");
}

TEST(Parser, ReadsComparisons) {
    EXPECT_EQ(shape("=1=2"), "(1=2)");
    EXPECT_EQ(shape("=1<>2"), "(1<>2)");
    EXPECT_EQ(shape("=1<2"), "(1<2)");
    EXPECT_EQ(shape("=1<=2"), "(1<=2)");
    EXPECT_EQ(shape("=1>2"), "(1>2)");
    EXPECT_EQ(shape("=1>=2"), "(1>=2)");
    EXPECT_EQ(shape("=1+2<3*4"), "((1+2)<(3*4))");
    EXPECT_EQ(shape("=(1<2)=1"), "((1<2)=1)");
}

TEST(Parser, ReadsRanges) {
    EXPECT_EQ(shape("=SUM(A1:B3)"), "SUM(A1:B3)");
    EXPECT_EQ(shape("=SUM(b3:$a$1)"), "SUM(B3:$A$1)");
    // разбирается, а #VALUE! будет уже при вычислении
    EXPECT_EQ(shape("=A1:B2"), "A1:B2");
}

TEST(Parser, ReadsFunctionCalls) {
    EXPECT_EQ(shape("=PI()"), "PI()");
    EXPECT_EQ(shape("=SUM(A1)"), "SUM(A1)");
    EXPECT_EQ(shape("=sum(a1)"), "SUM(A1)");
    EXPECT_EQ(shape("=IF(A1>0, 1, -1)"), "IF((A1>0),1,(-1))");
    EXPECT_EQ(shape("=SUM(1, MAX(A1:B2, 3)) * 2"), "(SUM(1,MAX(A1:B2,3))*2)");
    EXPECT_EQ(as_call(parse("=PI()")).args().size(), 0U);
    EXPECT_EQ(as_call(parse("=IF(1,2,3)")).args().size(), 3U);
}

TEST(Parser, FindsFunctionIgnoringCase) {
    const ExprPtr expr = parse("=sum(1)");
    const FunctionCallExpr& node = as_call(expr);
    EXPECT_EQ(node.name(), "sum");  // имя остаётся как написано
    ASSERT_NE(node.function(), nullptr);
    EXPECT_EQ(node.function()->name(), "SUM");
}

TEST(Parser, UnknownFunctionIsNotSyntaxError) {
    const ExprPtr expr = parse("=NOSUCH(1, 2)");
    const FunctionCallExpr& node = as_call(expr);
    EXPECT_EQ(node.name(), "NOSUCH");
    EXPECT_EQ(node.function(), nullptr);
    EXPECT_EQ(node.args().size(), 2U);
}

TEST(Parser, ReferenceOutsideSheetIsNotSyntaxError) {
    // номер строки за листом прижимается к первой строке за ним,
    // при вычислении это всё равно #REF!
    EXPECT_EQ(shape("=A99999999"), "A65537");
}

TEST(Parser, ReportsErrorPosition) {
    expect_error_at("=1+", 3);
    expect_error_at("=(1", 3);
    expect_error_at("=SUM(1,)", 7);
    expect_error_at("=1 2", 3);
    expect_error_at("=A1:", 4);
    expect_error_at("=A1:B", 4);
    expect_error_at("=A1:5", 4);
    expect_error_at("1+2", 0);
    expect_error_at("", 0);
    expect_error_at("=", 1);
    expect_error_at("=)", 1);
    expect_error_at("=ABC", 4);
    expect_error_at("=SUM(1 2)", 7);
    expect_error_at("=1<2<3", 4);
    expect_error_at("=1+*2", 3);
}

TEST(Parser, PassesLexerErrorsThrough) {
    expect_error_at("=1+#", 3);
    expect_error_at(R"(="abc)", 1);
}

TEST(Parser, RejectsNumbersOutOfRange) {
    expect_error_at("=1+1e999", 3);
}

TEST(Parser, LimitsNestingDepth) {
    const std::string ok = "=" + std::string(50, '(') + "1" + std::string(50, ')');
    EXPECT_EQ(shape(ok), "1");
    const std::string deep = "=" + std::string(1000, '(') + "1" + std::string(1000, ')');
    EXPECT_THROW(parse(deep), ParseError);
    EXPECT_THROW(parse("=" + std::string(1000, '-') + "1"), ParseError);
}

TEST(Parser, LimitsLength) {
    std::string formula = "=1";
    while (formula.size() <= Parser::max_length) {
        formula += "+1";
    }
    EXPECT_THROW(parse(formula), ParseError);
}

// Напечатанная формула разбирается в то же дерево: печать и разбор
// согласованы.
TEST(Parser, PrintedFormulaParsesBack) {
    const char* formulas[] = {
        "=1",
        "=-1",
        "=--A1",
        "=1+2*3",
        "=(1+2)*3",
        "=1-2-3",
        "=1-(2-3)",
        "=8/(4/2)",
        "=2*(3/4)",
        "=-(1+2)",
        "=-A1*2",
        "=2*-3",
        "=+1",
        "=0.1+0.2",
        "=1e-7*1e20",
        R"(="a""b")",
        R"(="")",
        "=a1",
        "=$A$1+A$2+$a3",
        "=SUM(A1:B3)",
        "=sum(a1:b3, 5)",
        "=PI()",
        "=IF(A1>0, \"pos\", \"neg\")",
        "=IF(A1, 1/0, 2)",
        "=MAX(SUM(1,2), -3) / 2",
        "=NOSUCH(1)",
        "=(1<2)=(3>4)",
        "=1+2<>3",
        "=(1+2)*(3-4)/(5+6)",
        "=A1:B2",
        "=((((1))))",
        "=1-(-1)",
    };
    for (const char* formula : formulas) {
        const std::string printed = FormulaPrinter::print(*parse(formula));
        const ExprPtr reparsed = parse(printed);
        EXPECT_EQ(FormulaPrinter::print(*reparsed), printed) << formula;
        EXPECT_EQ(ShapePrinter::print(*reparsed), shape(formula)) << formula;
    }
}
