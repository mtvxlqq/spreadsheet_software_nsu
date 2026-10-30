#include <compare>
#include <optional>

#include <gtest/gtest.h>

#include "core/number_format.hpp"
#include "core/value.hpp"

using namespace ss::core;

namespace {

std::optional<std::partial_ordering> compare(const ValuePtr& lhs, const ValuePtr& rhs) {
    return lhs->compare(*rhs);
}

}  // namespace

TEST(GeneralFormat, ShowsUpToTenSignificantDigits) {
    const GeneralFormat format;
    EXPECT_EQ(format.format(3), "3");
    EXPECT_EQ(format.format(-7), "-7");
    EXPECT_EQ(format.format(2.5), "2.5");
    EXPECT_EQ(format.format(0.1), "0.1");
    EXPECT_EQ(format.format(0.1 + 0.2), "0.3");
    EXPECT_EQ(format.format(1.0 / 3), "0.3333333333");
    EXPECT_EQ(format.format(1234567890), "1234567890");
    EXPECT_EQ(format.format(1e20), "1e+20");
}

TEST(GeneralFormat, ShowsNegativeZeroAsZero) {
    EXPECT_EQ(GeneralFormat{}.format(-0.0), "0");
}

TEST(NumberValue, BehavesAsNumber) {
    const ValuePtr value = makeNumber(0.1);
    EXPECT_EQ(value->asNumber(), 0.1);
    EXPECT_EQ(value->toText(), "0.1");
    EXPECT_EQ(value->display(GeneralFormat{}), "0.1");
    EXPECT_FALSE(value->isError());
    EXPECT_FALSE(value->isEmpty());
}

TEST(StringValue, IsNotNumberEvenWhenLooksLikeOne) {
    const ValuePtr value = makeString("123");
    EXPECT_EQ(value->asNumber(), std::nullopt);
    EXPECT_EQ(value->toText(), "123");
    EXPECT_EQ(value->display(GeneralFormat{}), "123");
    EXPECT_FALSE(value->isError());
    EXPECT_FALSE(value->isEmpty());
}

TEST(ErrorValue, ShowsSpreadsheetErrorText) {
    EXPECT_EQ(makeError(ErrorCode::DIV0)->toText(), "#DIV/0!");
    EXPECT_EQ(makeError(ErrorCode::REF)->toText(), "#REF!");
    EXPECT_EQ(makeError(ErrorCode::NAME)->toText(), "#NAME?");
    EXPECT_EQ(makeError(ErrorCode::CYCLE)->toText(), "#CYCLE!");
    EXPECT_EQ(makeError(ErrorCode::VALUE)->display(GeneralFormat{}), "#VALUE!");
}

TEST(ErrorValue, AnswersEveryBaseMethodWithoutThrowing) {
    const ValuePtr value = makeError(ErrorCode::REF);
    EXPECT_TRUE(value->isError());
    EXPECT_FALSE(value->isEmpty());
    EXPECT_EQ(value->asNumber(), std::nullopt);
}

TEST(EmptyValue, IsZeroInArithmeticAndBlankOnScreen) {
    const ValuePtr value = emptyValue();
    EXPECT_EQ(value->asNumber(), 0.0);
    EXPECT_EQ(value->toText(), "");
    EXPECT_EQ(value->display(GeneralFormat{}), "");
    EXPECT_TRUE(value->isEmpty());
    EXPECT_FALSE(value->isError());
}

TEST(EmptyValue, IsSharedInstance) {
    EXPECT_EQ(emptyValue(), emptyValue());
}

TEST(ValueCompare, Numbers) {
    EXPECT_EQ(compare(makeNumber(1), makeNumber(2)), std::partial_ordering::less);
    EXPECT_EQ(compare(makeNumber(2), makeNumber(2)), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(makeNumber(3), makeNumber(2)), std::partial_ordering::greater);
}

TEST(ValueCompare, StringsIgnoreCase) {
    EXPECT_EQ(compare(makeString("abc"), makeString("ABC")), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(makeString("a"), makeString("b")), std::partial_ordering::less);
    EXPECT_EQ(compare(makeString("B"), makeString("a")), std::partial_ordering::greater);
}

TEST(ValueCompare, NumberAndStringAreIncomparable) {
    EXPECT_EQ(compare(makeNumber(1), makeString("1")), std::nullopt);
    EXPECT_EQ(compare(makeString("1"), makeNumber(1)), std::nullopt);
}

TEST(ValueCompare, EmptyIsZeroForNumbers) {
    EXPECT_EQ(compare(emptyValue(), makeNumber(0)), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(emptyValue(), makeNumber(5)), std::partial_ordering::less);
    EXPECT_EQ(compare(makeNumber(-1), emptyValue()), std::partial_ordering::less);
}

TEST(ValueCompare, EmptyIsBlankStringForStrings) {
    EXPECT_EQ(compare(emptyValue(), makeString("")), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(emptyValue(), makeString("a")), std::partial_ordering::less);
    EXPECT_EQ(compare(makeString("a"), emptyValue()), std::partial_ordering::greater);
}

TEST(ValueCompare, EmptyEqualsEmpty) {
    EXPECT_EQ(compare(emptyValue(), emptyValue()), std::partial_ordering::equivalent);
}

TEST(ValueCompare, ErrorsAreIncomparable) {
    const ValuePtr error = makeError(ErrorCode::DIV0);
    EXPECT_EQ(compare(error, makeNumber(1)), std::nullopt);
    EXPECT_EQ(compare(makeNumber(1), error), std::nullopt);
    EXPECT_EQ(compare(error, makeString("a")), std::nullopt);
    EXPECT_EQ(compare(makeString("a"), error), std::nullopt);
    EXPECT_EQ(compare(error, emptyValue()), std::nullopt);
    EXPECT_EQ(compare(emptyValue(), error), std::nullopt);
    EXPECT_EQ(compare(error, error), std::nullopt);
}
