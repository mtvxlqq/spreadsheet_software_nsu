#include <chrono>
#include <compare>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/date.hpp"
#include "core/function_registry.hpp"
#include "core/io/csv_reader.hpp"
#include "core/io/native_format.hpp"
#include "core/number_format.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "core/value_parser.hpp"

using ss::core::CellAddress;
using ss::core::Date;
using ss::core::date_from_parts;
using ss::core::format_date;
using ss::core::make_date;
using ss::core::parse_date;
using ss::core::shift_date;
using ss::core::Spreadsheet;
using ss::core::ValuePtr;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

Date day(int year, int month, int number) {
    return date_from_parts(year, month, number).value();
}

// "2024-03-15" для даты и "нет" для nullopt, так сравнивать нагляднее
std::string parsed(std::string_view text) {
    const std::optional<Date> date = parse_date(text);
    return date ? format_date(*date) : "нет";
}

class DateTest : public ::testing::Test {
protected:
    DateTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
    }

    void set(std::string_view address, std::string_view raw) {
        sheet.set_input(at(address), raw);
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    // результат формулы, которую кладём в Z1
    std::string eval(std::string_view formula) {
        set("Z1", formula);
        return shown("Z1");
    }

    bool is_date(std::string_view address) const {
        return sheet.value(at(address))->as_date().has_value();
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    Spreadsheet sheet{functions, value_parsers};
};

}  // namespace

TEST(DateParsing, IsoFormat) {
    EXPECT_EQ(parsed("2024-03-15"), "2024-03-15");
    EXPECT_EQ(parsed("1999-12-31"), "1999-12-31");
    EXPECT_EQ(parsed("2024-3-5"), "2024-03-05");
    EXPECT_EQ(parsed("0001-01-01"), "0001-01-01");
    EXPECT_EQ(parsed("9999-12-31"), "9999-12-31");
}

TEST(DateParsing, DottedFormat) {
    EXPECT_EQ(parsed("15.03.2024"), "2024-03-15");
    EXPECT_EQ(parsed("5.3.2024"), "2024-03-05");
    EXPECT_EQ(parsed("31.12.1999"), "1999-12-31");
}

TEST(DateParsing, LeapYears) {
    EXPECT_EQ(parsed("29.02.2024"), "2024-02-29");
    EXPECT_EQ(parsed("29.02.2023"), "нет");
    // вековой год високосный, только если делится на 400
    EXPECT_EQ(parsed("29.02.2000"), "2000-02-29");
    EXPECT_EQ(parsed("29.02.1900"), "нет");
}

TEST(DateParsing, DatesThatDoNotExist) {
    for (const char* text : {"31.02.2024", "31.04.2024", "32.01.2024", "00.01.2024", "15.13.2024",
                             "15.00.2024", "2024-13-01", "2024-00-10", "2024-02-30", "0000-01-01"}) {
        EXPECT_EQ(parsed(text), "нет") << text;
    }
}

TEST(DateParsing, NotDatesAtAll) {
    for (const char* text :
         {"", "2024", "2024-03", "2024-03-15-01", "15.03", "15.03.24", "24-03-15", "1.2.3",
          "15/03/2024", "2024.03.15", "15-03-2024", "2024-03-1a", " 2024-03-15", "2024-03-15 ",
          "+2024-03-15", "2024--03-15", "15..2024", "20240-03-15", "2024-003-15", "abc"}) {
        EXPECT_EQ(parsed(text), "нет") << text;
    }
}

TEST(DateParsing, PartsOutsideLimits) {
    EXPECT_FALSE(date_from_parts(0, 1, 1).has_value());
    EXPECT_FALSE(date_from_parts(10000, 1, 1).has_value());
    EXPECT_FALSE(date_from_parts(-5, 1, 1).has_value());
    EXPECT_FALSE(date_from_parts(2024, -1, 1).has_value());
    EXPECT_FALSE(date_from_parts(2024, 1, 100).has_value());
    EXPECT_TRUE(date_from_parts(1, 1, 1).has_value());
    EXPECT_TRUE(date_from_parts(9999, 12, 31).has_value());
}

TEST(DateParsing, EpochIsDayZero) {
    EXPECT_EQ(day(1970, 1, 1).time_since_epoch().count(), 0);
    EXPECT_EQ(day(1970, 1, 2).time_since_epoch().count(), 1);
    EXPECT_EQ(day(1969, 12, 31).time_since_epoch().count(), -1);
}

TEST(DateShift, MovesByDays) {
    EXPECT_EQ(format_date(shift_date(day(2024, 1, 31), 1).value()), "2024-02-01");
    EXPECT_EQ(format_date(shift_date(day(2024, 12, 31), 1).value()), "2025-01-01");
    EXPECT_EQ(format_date(shift_date(day(2024, 3, 1), -1).value()), "2024-02-29");
    EXPECT_EQ(format_date(shift_date(day(2024, 3, 15), 0).value()), "2024-03-15");
}

TEST(DateShift, StopsAtYearLimits) {
    EXPECT_TRUE(shift_date(day(9999, 12, 30), 1).has_value());
    EXPECT_FALSE(shift_date(day(9999, 12, 31), 1).has_value());
    EXPECT_TRUE(shift_date(day(1, 1, 2), -1).has_value());
    EXPECT_FALSE(shift_date(day(1, 1, 1), -1).has_value());
    // огромные числа не должны переполнить счёт
    EXPECT_FALSE(shift_date(day(2024, 1, 1), 9'000'000'000'000'000'000LL).has_value());
    EXPECT_FALSE(shift_date(day(2024, 1, 1), -9'000'000'000'000'000'000LL).has_value());
}

TEST(DateValueTest, TextNumberAndDisplay) {
    const ValuePtr value = make_date(day(2024, 3, 15));
    EXPECT_EQ(value->to_text(), "2024-03-15");
    EXPECT_EQ(value->as_date(), day(2024, 3, 15));
    EXPECT_EQ(value->as_number(), 19797.0);
    EXPECT_FALSE(value->is_error());
    EXPECT_FALSE(value->is_empty());
    // формат чисел на дату не действует
    EXPECT_EQ(value->display(ss::core::FixedFormat(2)), "2024-03-15");
    EXPECT_EQ(value->display(ss::core::PercentFormat(0)), "2024-03-15");
}

TEST(DateValueTest, OtherValuesAreNotDates) {
    EXPECT_FALSE(ss::core::make_number(19797)->as_date().has_value());
    EXPECT_FALSE(ss::core::make_string("2024-03-15")->as_date().has_value());
    EXPECT_FALSE(ss::core::empty_value()->as_date().has_value());
    EXPECT_FALSE(ss::core::make_error(ss::core::ErrorCode::Value)->as_date().has_value());
}

TEST(DateValueTest, ComparesWithDatesAndEmpty) {
    const ValuePtr early = make_date(day(2024, 1, 1));
    const ValuePtr late = make_date(day(2024, 12, 31));
    EXPECT_EQ(early->compare(*late), std::partial_ordering::less);
    EXPECT_EQ(late->compare(*early), std::partial_ordering::greater);
    EXPECT_EQ(early->compare(*make_date(day(2024, 1, 1))), std::partial_ordering::equivalent);
    // пустая ячейка раньше любой даты, с какой стороны ни сравнивай
    EXPECT_EQ(early->compare(*ss::core::empty_value()), std::partial_ordering::greater);
    EXPECT_EQ(ss::core::empty_value()->compare(*early), std::partial_ordering::less);
    // а с числом и строкой дату не сравнить
    EXPECT_EQ(early->compare(*ss::core::make_number(5)), std::nullopt);
    EXPECT_EQ(ss::core::make_number(5)->compare(*early), std::nullopt);
    EXPECT_EQ(early->compare(*ss::core::make_string("2024-01-01")), std::nullopt);
    EXPECT_EQ(ss::core::make_string("x")->compare(*early), std::nullopt);
}

TEST_F(DateTest, InputInBothFormatsBecomesDate) {
    set("A1", "2024-03-15");
    set("A2", "15.03.2024");
    set("A3", "  5.3.2024 ");
    EXPECT_TRUE(is_date("A1"));
    EXPECT_TRUE(is_date("A2"));
    EXPECT_EQ(shown("A1"), "2024-03-15");
    // показываем всегда в одном виде, а в raw остаётся то, что ввели
    EXPECT_EQ(shown("A2"), "2024-03-15");
    EXPECT_EQ(sheet.raw(at("A2")), "15.03.2024");
    EXPECT_EQ(shown("A3"), "2024-03-05");
}

TEST_F(DateTest, DateThatDoesNotExistStaysText) {
    set("A1", "31.02.2024");
    set("A2", "2023-02-29");
    EXPECT_FALSE(is_date("A1"));
    EXPECT_EQ(shown("A1"), "31.02.2024");
    EXPECT_EQ(sheet.value(at("A1"))->as_number(), std::nullopt);
    EXPECT_FALSE(is_date("A2"));
}

TEST_F(DateTest, ApostropheKeepsDateAsText) {
    set("A1", "'2024-03-15");
    EXPECT_FALSE(is_date("A1"));
    EXPECT_EQ(shown("A1"), "2024-03-15");
}

TEST_F(DateTest, NumbersAreStillNumbers) {
    set("A1", "2024");
    set("A2", "15.03");
    set("A3", "-5");
    EXPECT_FALSE(is_date("A1"));
    EXPECT_EQ(sheet.value(at("A1"))->as_number(), 2024.0);
    EXPECT_EQ(sheet.value(at("A2"))->as_number(), 15.03);
    EXPECT_EQ(sheet.value(at("A3"))->as_number(), -5.0);
}

TEST_F(DateTest, DifferenceOfDatesIsNumberOfDays) {
    set("A1", "2024-03-15");
    set("B1", "2024-03-01");
    set("C1", "=A1-B1");
    EXPECT_EQ(shown("C1"), "14");
    EXPECT_FALSE(is_date("C1"));
    EXPECT_EQ(eval("=B1-A1"), "-14");
    EXPECT_EQ(eval("=A1-A1"), "0");
    // через високосный февраль и через год
    set("A2", "01.03.2024");
    set("B2", "01.02.2024");
    EXPECT_EQ(eval("=A2-B2"), "29");
    set("A3", "2025-01-01");
    set("B3", "2024-01-01");
    EXPECT_EQ(eval("=A3-B3"), "366");
}

TEST_F(DateTest, DifferenceFollowsEdits) {
    set("A1", "2024-03-15");
    set("B1", "2024-03-01");
    set("C1", "=A1-B1");
    set("B1", "2024-03-10");
    EXPECT_EQ(shown("C1"), "5");
}

TEST_F(DateTest, DatePlusNumberIsDate) {
    set("A1", "2024-01-31");
    set("B1", "=A1+1");
    EXPECT_TRUE(is_date("B1"));
    EXPECT_EQ(shown("B1"), "2024-02-01");
    // число слева тоже можно
    EXPECT_EQ(eval("=30+A1"), "2024-03-01");
    EXPECT_EQ(eval("=A1+366"), "2025-01-31");
    EXPECT_EQ(eval("=A1+0"), "2024-01-31");
}

TEST_F(DateTest, DateMinusNumberIsDate) {
    set("A1", "2024-03-01");
    EXPECT_EQ(eval("=A1-1"), "2024-02-29");
    EXPECT_EQ(eval("=A1-366"), "2023-03-01");
    EXPECT_EQ(eval("=A1+-1"), "2024-02-29");
}

TEST_F(DateTest, FractionOfDayIsDropped) {
    set("A1", "2024-03-01");
    EXPECT_EQ(eval("=A1+1.9"), "2024-03-02");
    EXPECT_EQ(eval("=A1-0.5"), "2024-03-01");
}

TEST_F(DateTest, ChainOfDateArithmetic) {
    set("A1", "2024-03-15");
    set("A2", "=A1+10");
    set("A3", "=A2-A1");
    set("A4", "=A2+A3");
    EXPECT_EQ(shown("A2"), "2024-03-25");
    EXPECT_EQ(shown("A3"), "10");
    EXPECT_EQ(shown("A4"), "2024-04-04");
}

TEST_F(DateTest, TwoDatesCannotBeAdded) {
    set("A1", "2024-03-15");
    set("B1", "2024-03-01");
    EXPECT_EQ(eval("=A1+B1"), "#VALUE!");
}

TEST_F(DateTest, DateAndTextGiveValueError) {
    set("A1", "2024-03-15");
    set("B1", "text");
    EXPECT_EQ(eval("=A1+B1"), "#VALUE!");
    EXPECT_EQ(eval("=A1-B1"), "#VALUE!");
    EXPECT_EQ(eval("=B1+A1"), "#VALUE!");
    EXPECT_EQ(eval("=A1+\"x\""), "#VALUE!");
}

TEST_F(DateTest, ErrorsPassThroughDateArithmetic) {
    set("A1", "2024-03-15");
    EXPECT_EQ(eval("=A1+1/0"), "#DIV/0!");
    EXPECT_EQ(eval("=1/0-A1"), "#DIV/0!");
}

TEST_F(DateTest, EmptyCellCountsAsZeroDays) {
    set("A1", "2024-03-15");
    EXPECT_EQ(eval("=A1+B5"), "2024-03-15");
    EXPECT_EQ(eval("=A1-B5"), "2024-03-15");
    EXPECT_EQ(eval("=B5+A1"), "2024-03-15");
}

TEST_F(DateTest, OutOfYearRangeIsValueError) {
    set("A1", "9999-12-31");
    set("A2", "0001-01-01");
    EXPECT_EQ(eval("=A1+1"), "#VALUE!");
    EXPECT_EQ(eval("=A2-1"), "#VALUE!");
    EXPECT_EQ(eval("=A1-1"), "9999-12-30");
    EXPECT_EQ(eval("=A2+1e300"), "#VALUE!");
    EXPECT_EQ(eval("=A2-1e300"), "#VALUE!");
    // а разность крайних дат обычное число
    EXPECT_EQ(eval("=A1-A2"), "3652058");
}

// в остальных операциях дата ведёт себя как число дней
TEST_F(DateTest, OtherOperationsUseNumberOfDays) {
    set("A1", "1970-01-11");
    EXPECT_EQ(eval("=A1*2"), "20");
    EXPECT_EQ(eval("=A1/5"), "2");
    EXPECT_EQ(eval("=-A1"), "-10");
    EXPECT_EQ(eval("=5-A1"), "-5");
}

TEST_F(DateTest, DatesAreCompared) {
    set("A1", "2024-03-15");
    set("B1", "2024-03-01");
    set("C1", "15.03.2024");
    EXPECT_EQ(eval("=A1>B1"), "1");
    EXPECT_EQ(eval("=A1<B1"), "0");
    EXPECT_EQ(eval("=A1=C1"), "1");
    EXPECT_EQ(eval("=A1<>B1"), "1");
    EXPECT_EQ(eval("=A1>=C1"), "1");
    EXPECT_EQ(eval("=IF(A1>B1, \"позже\", \"раньше\")"), "позже");
}

TEST_F(DateTest, DateIsComparedWithEmptyButNotWithNumber) {
    set("A1", "2024-03-15");
    EXPECT_EQ(eval("=A1>B5"), "1");
    EXPECT_EQ(eval("=B5<A1"), "1");
    EXPECT_EQ(eval("=A1=B5"), "0");
    EXPECT_EQ(eval("=A1>5"), "#VALUE!");
    EXPECT_EQ(eval("=A1=\"2024-03-15\""), "#VALUE!");
}

// функции работают без правок: для них дата это число дней
TEST_F(DateTest, AggregateFunctionsSeeDaysAsNumbers) {
    set("A1", "1970-01-02");
    set("A2", "1970-01-04");
    set("A3", "1970-01-11");
    EXPECT_EQ(eval("=SUM(A1:A3)"), "14");
    EXPECT_EQ(eval("=MIN(A1:A3)"), "1");
    EXPECT_EQ(eval("=MAX(A1:A3)"), "10");
    EXPECT_EQ(eval("=AVERAGE(A1:A2)"), "2");
    // самую позднюю дату можно получить обратно как дату
    EXPECT_EQ(eval("=DATE(1970,1,1)+MAX(A1:A3)"), "1970-01-11");
}

TEST_F(DateTest, DateFunctionBuildsDate) {
    set("A1", "=DATE(2024, 3, 15)");
    EXPECT_TRUE(is_date("A1"));
    EXPECT_EQ(shown("A1"), "2024-03-15");
    EXPECT_EQ(eval("=date(2024,2,29)"), "2024-02-29");
    set("B1", "2024");
    set("B2", "12");
    set("B3", "31");
    EXPECT_EQ(eval("=DATE(B1, B2, B3)"), "2024-12-31");
    EXPECT_EQ(eval("=DATE(2024,3,15)-DATE(2024,3,1)"), "14");
    EXPECT_EQ(eval("=DATE(2024,3,15)=A1"), "1");
}

TEST_F(DateTest, DateFunctionRejectsBadArguments) {
    for (const char* formula :
         {"=DATE(2024,2,31)", "=DATE(2023,2,29)", "=DATE(2024,13,1)", "=DATE(2024,0,1)",
          "=DATE(0,1,1)", "=DATE(10000,1,1)", "=DATE(2024,1)", "=DATE()", "=DATE(2024,1,1,1)",
          "=DATE(\"2024\",1,1)", "=DATE(2024.5,1,1)", "=DATE(1e300,1,1)", "=DATE(A1:A3,1,1)"}) {
        EXPECT_EQ(eval(formula), "#VALUE!") << formula;
    }
    EXPECT_EQ(eval("=DATE(1/0,1,1)"), "#DIV/0!");
}

TEST_F(DateTest, SavedAndLoadedAsDate) {
    set("A1", "15.03.2024");
    set("A2", "=A1+1");
    const ss::core::NativeFormat format;
    std::ostringstream out;
    format.save(out, sheet);
    EXPECT_EQ(out.str(), "SST 2\nA1\t15.03.2024\nA2\t=A1+1\n");

    Spreadsheet loaded{functions, value_parsers};
    std::istringstream in(out.str());
    format.load(in, loaded);
    EXPECT_TRUE(loaded.value(at("A1"))->as_date().has_value());
    EXPECT_EQ(loaded.value(at("A2"))->to_text(), "2024-03-16");
}

TEST_F(DateTest, CsvImportRecognizesDates) {
    std::istringstream in("2024-03-15,31.02.2024,5\n");
    ss::core::CsvReader().load(in, sheet);
    EXPECT_TRUE(is_date("A1"));
    EXPECT_FALSE(is_date("B1"));
    EXPECT_EQ(shown("B1"), "31.02.2024");
    EXPECT_EQ(eval("=A1+C1"), "2024-03-20");
}

TEST(ValueParserRegistryTest, EmptyRegistryKnowsNothing) {
    const ss::core::ValueParserRegistry registry;
    EXPECT_EQ(registry.parse("12"), nullptr);
    EXPECT_EQ(registry.parse("2024-03-15"), nullptr);
}

TEST(ValueParserRegistryTest, BuiltinsReadNumbersAndDates) {
    ss::core::ValueParserRegistry registry;
    ss::core::register_value_parsers(registry);
    EXPECT_EQ(registry.parse("12")->as_number(), 12.0);
    EXPECT_TRUE(registry.parse("2024-03-15")->as_date().has_value());
    EXPECT_EQ(registry.parse("hello"), nullptr);
    EXPECT_EQ(registry.parse("31.02.2024"), nullptr);
}

// без реестра таблица понимает только формулы и текст
TEST(ValueParserRegistryTest, SheetWithoutParsersKeepsEverythingAsText) {
    ss::core::FunctionRegistry functions;
    const ss::core::ValueParserRegistry nothing;
    Spreadsheet sheet{functions, nothing};
    sheet.set_input(at("A1"), "12");
    sheet.set_input(at("A2"), "=1+2");
    EXPECT_EQ(sheet.value(at("A1"))->as_number(), std::nullopt);
    EXPECT_EQ(sheet.value(at("A1"))->to_text(), "12");
    EXPECT_EQ(sheet.value(at("A2"))->to_text(), "3");
}

namespace {

// узнаёт всё, что начинается с #, и говорит, что это число 7
class SevenParser final : public ss::core::ValueParser {
public:
    ValuePtr parse(std::string_view text) const override {
        return text.starts_with('#') ? ss::core::make_number(7) : nullptr;
    }
};

}  // namespace

TEST(ValueParserRegistryTest, FirstParserThatKnowsWins) {
    ss::core::ValueParserRegistry registry;
    registry.add(std::make_shared<const SevenParser>());
    ss::core::register_value_parsers(registry);
    EXPECT_EQ(registry.parse("#abc")->as_number(), 7.0);
    EXPECT_EQ(registry.parse("12")->as_number(), 12.0);
}
