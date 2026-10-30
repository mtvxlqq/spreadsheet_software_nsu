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
    const ValuePtr value = make_number(0.1);
    EXPECT_EQ(value->as_number(), 0.1);
    EXPECT_EQ(value->to_text(), "0.1");
    EXPECT_EQ(value->display(GeneralFormat{}), "0.1");
    EXPECT_FALSE(value->is_error());
    EXPECT_FALSE(value->is_empty());
}

TEST(StringValue, IsNotNumberEvenWhenLooksLikeOne) {
    const ValuePtr value = make_string("123");
    EXPECT_EQ(value->as_number(), std::nullopt);
    EXPECT_EQ(value->to_text(), "123");
    EXPECT_EQ(value->display(GeneralFormat{}), "123");
    EXPECT_FALSE(value->is_error());
    EXPECT_FALSE(value->is_empty());
}

TEST(ErrorValue, ShowsSpreadsheetErrorText) {
    EXPECT_EQ(make_error(ErrorCode::Div0)->to_text(), "#DIV/0!");
    EXPECT_EQ(make_error(ErrorCode::Ref)->to_text(), "#REF!");
    EXPECT_EQ(make_error(ErrorCode::Name)->to_text(), "#NAME?");
    EXPECT_EQ(make_error(ErrorCode::Cycle)->to_text(), "#CYCLE!");
    EXPECT_EQ(make_error(ErrorCode::Value)->display(GeneralFormat{}), "#VALUE!");
}

TEST(ErrorValue, AnswersEveryBaseMethodWithoutThrowing) {
    const ValuePtr value = make_error(ErrorCode::Ref);
    EXPECT_TRUE(value->is_error());
    EXPECT_FALSE(value->is_empty());
    EXPECT_EQ(value->as_number(), std::nullopt);
}

TEST(EmptyValue, IsZeroInArithmeticAndBlankOnScreen) {
    const ValuePtr value = empty_value();
    EXPECT_EQ(value->as_number(), 0.0);
    EXPECT_EQ(value->to_text(), "");
    EXPECT_EQ(value->display(GeneralFormat{}), "");
    EXPECT_TRUE(value->is_empty());
    EXPECT_FALSE(value->is_error());
}

TEST(EmptyValue, IsSharedInstance) {
    EXPECT_EQ(empty_value(), empty_value());
}

TEST(ValueCompare, Numbers) {
    EXPECT_EQ(compare(make_number(1), make_number(2)), std::partial_ordering::less);
    EXPECT_EQ(compare(make_number(2), make_number(2)), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(make_number(3), make_number(2)), std::partial_ordering::greater);
}

TEST(ValueCompare, StringsIgnoreCase) {
    EXPECT_EQ(compare(make_string("abc"), make_string("ABC")), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(make_string("a"), make_string("b")), std::partial_ordering::less);
    EXPECT_EQ(compare(make_string("B"), make_string("a")), std::partial_ordering::greater);
}

TEST(ValueCompare, NumberAndStringAreIncomparable) {
    EXPECT_EQ(compare(make_number(1), make_string("1")), std::nullopt);
    EXPECT_EQ(compare(make_string("1"), make_number(1)), std::nullopt);
}

TEST(ValueCompare, EmptyIsZeroForNumbers) {
    EXPECT_EQ(compare(empty_value(), make_number(0)), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(empty_value(), make_number(5)), std::partial_ordering::less);
    EXPECT_EQ(compare(make_number(-1), empty_value()), std::partial_ordering::less);
}

TEST(ValueCompare, EmptyIsBlankStringForStrings) {
    EXPECT_EQ(compare(empty_value(), make_string("")), std::partial_ordering::equivalent);
    EXPECT_EQ(compare(empty_value(), make_string("a")), std::partial_ordering::less);
    EXPECT_EQ(compare(make_string("a"), empty_value()), std::partial_ordering::greater);
}

TEST(ValueCompare, EmptyEqualsEmpty) {
    EXPECT_EQ(compare(empty_value(), empty_value()), std::partial_ordering::equivalent);
}

TEST(ValueCompare, ErrorsAreIncomparable) {
    const ValuePtr error = make_error(ErrorCode::Div0);
    EXPECT_EQ(compare(error, make_number(1)), std::nullopt);
    EXPECT_EQ(compare(make_number(1), error), std::nullopt);
    EXPECT_EQ(compare(error, make_string("a")), std::nullopt);
    EXPECT_EQ(compare(make_string("a"), error), std::nullopt);
    EXPECT_EQ(compare(error, empty_value()), std::nullopt);
    EXPECT_EQ(compare(empty_value(), error), std::nullopt);
    EXPECT_EQ(compare(error, error), std::nullopt);
}
