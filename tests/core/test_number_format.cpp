#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/errors.hpp"
#include "core/number_format.hpp"
#include "core/number_format_registry.hpp"

using ss::core::FixedFormat;
using ss::core::NumberFormatRegistry;
using ss::core::PercentFormat;

namespace {

class NumberFormatRegistryTest : public ::testing::Test {
protected:
    NumberFormatRegistryTest() {
        ss::core::register_number_formats(formats);
    }

    std::string shown(std::string_view spec, double value) const {
        return formats.create(spec)->format(value);
    }

    NumberFormatRegistry formats;
};

}  // namespace

TEST(FixedFormatTest, RoundsToGivenDigits) {
    EXPECT_EQ(FixedFormat(2).format(3.14159), "3.14");
    EXPECT_EQ(FixedFormat(2).format(2.0 / 3), "0.67");
    EXPECT_EQ(FixedFormat(0).format(2.6), "3");
    EXPECT_EQ(FixedFormat(4).format(1), "1.0000");
}

// половина округляется от нуля, а не к чётному
TEST(FixedFormatTest, HalfIsRoundedAwayFromZero) {
    EXPECT_EQ(FixedFormat(0).format(2.5), "3");
    EXPECT_EQ(FixedFormat(0).format(3.5), "4");
    EXPECT_EQ(FixedFormat(0).format(-2.5), "-3");
    EXPECT_EQ(FixedFormat(1).format(0.25), "0.3");
    EXPECT_EQ(PercentFormat(0).format(0.125), "13%");
}

TEST(FixedFormatTest, HugeNumberIsNotBroken) {
    EXPECT_EQ(FixedFormat(0).format(1e20), "100000000000000000000");
    EXPECT_EQ(FixedFormat(15).format(1e300).size(), 301U + 16U);
}

TEST(FixedFormatTest, ZeroAndNegativeNumbers) {
    EXPECT_EQ(FixedFormat(2).format(0), "0.00");
    EXPECT_EQ(FixedFormat(2).format(-1.5), "-1.50");
    EXPECT_EQ(FixedFormat(1).format(-12.34), "-12.3");
}

// -0.001 округляется до нуля, и минус перед ним только путал бы
TEST(FixedFormatTest, RoundedToZeroHasNoMinus) {
    EXPECT_EQ(FixedFormat(2).format(-0.001), "0.00");
    EXPECT_EQ(FixedFormat(0).format(-0.4), "0");
    EXPECT_EQ(FixedFormat(2).format(-0.0), "0.00");
    EXPECT_EQ(FixedFormat(2).format(-0.006), "-0.01");
}

TEST(PercentFormatTest, MultipliesByHundred) {
    EXPECT_EQ(PercentFormat(1).format(0.256), "25.6%");
    EXPECT_EQ(PercentFormat(0).format(0.5), "50%");
    EXPECT_EQ(PercentFormat(2).format(1), "100.00%");
    EXPECT_EQ(PercentFormat(0).format(0), "0%");
    EXPECT_EQ(PercentFormat(1).format(-0.125), "-12.5%");
    EXPECT_EQ(PercentFormat(1).format(-0.0001), "0.0%");
}

TEST_F(NumberFormatRegistryTest, CreatesFormatBySpec) {
    EXPECT_EQ(shown("fixed:2", 1.005), "1.00");
    EXPECT_EQ(shown("fixed:0", 7.5), "8");
    EXPECT_EQ(shown("percent:1", 0.5), "50.0%");
}

TEST_F(NumberFormatRegistryTest, EmptySpecIsGeneral) {
    EXPECT_EQ(shown("", 1.0 / 3), "0.3333333333");
}

// неизвестный формат не ошибка: число просто показывается как обычно
TEST_F(NumberFormatRegistryTest, UnknownOrBrokenSpecIsGeneral) {
    EXPECT_EQ(shown("money:2", 0.5), "0.5");
    EXPECT_EQ(shown("fixed", 0.5), "0.5");
    EXPECT_EQ(shown("fixed:", 0.5), "0.5");
    EXPECT_EQ(shown("fixed:abc", 0.5), "0.5");
    EXPECT_EQ(shown("fixed:-1", 0.5), "0.5");
    EXPECT_EQ(shown("fixed:99", 0.5), "0.5");
    EXPECT_EQ(shown("fixed:2:3", 0.5), "0.5");
    EXPECT_EQ(shown(":2", 0.5), "0.5");
}

TEST_F(NumberFormatRegistryTest, MakeSpecBuildsTextForStyle) {
    EXPECT_EQ(formats.make_spec("fixed", 2), "fixed:2");
    EXPECT_EQ(formats.make_spec("percent", 0), "percent:0");
    EXPECT_EQ(formats.make_spec("fixed", NumberFormatRegistry::max_digits), "fixed:15");
}

TEST_F(NumberFormatRegistryTest, MakeSpecRejectsUnknownNameAndBadDigits) {
    EXPECT_THROW(formats.make_spec("money", 2), ss::core::Error);
    EXPECT_THROW(formats.make_spec("fixed", -1), ss::core::Error);
    EXPECT_THROW(formats.make_spec("fixed", 16), ss::core::Error);
    EXPECT_THROW(formats.make_spec("", 2), ss::core::Error);
}

TEST_F(NumberFormatRegistryTest, NamesAreSorted) {
    EXPECT_EQ(formats.names(), (std::vector<std::string>{"fixed", "percent"}));
}

TEST_F(NumberFormatRegistryTest, SameNameTwiceIsError) {
    EXPECT_THROW(
        formats.add("fixed", [](int digits) { return std::make_shared<const FixedFormat>(digits); }),
        ss::core::Error);
}
