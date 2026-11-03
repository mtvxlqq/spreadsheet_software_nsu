#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/csv_reader.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "support/printers.hpp"

using ss::core::CellAddress;
using ss::core::CsvReader;
using ss::core::FileFormatError;
using ss::core::Range;
using ss::core::Spreadsheet;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class CsvTest : public ::testing::Test {
protected:
    CsvTest() {
        ss::core::register_builtins(functions);
    }

    void load(std::string_view text) {
        std::istringstream in{std::string(text)};
        reader.load(in, sheet);
    }

    void load_file(std::string_view name) {
        std::ifstream in(std::string(SS_TESTDATA_DIR) + "/csv/" + std::string(name),
                         std::ios::binary);
        ASSERT_TRUE(in.is_open()) << name;
        reader.load(in, sheet);
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    Spreadsheet sheet{functions};
    CsvReader reader;
};

}  // namespace

TEST_F(CsvTest, PlainFile) {
    load_file("plain.csv");
    EXPECT_EQ(sheet.used_range(), Range(at("A1"), at("C3")));
    EXPECT_EQ(shown("A1"), "name");
    EXPECT_EQ(shown("B2"), "25");
    EXPECT_TRUE(sheet.value(at("B2"))->as_number().has_value());
    EXPECT_EQ(shown("C3"), "Novosibirsk");
}

TEST_F(CsvTest, QuotesAndEscapedQuotes) {
    load_file("quoted.csv");
    EXPECT_EQ(shown("A1"), "a, b");
    EXPECT_EQ(shown("B1"), "say \"hi\"");
    EXPECT_EQ(shown("C1"), "plain");
    // "" это пустое поле, ячейки нет
    EXPECT_EQ(sheet.raw(at("A2")), "");
}

TEST_F(CsvTest, FormulaStaysText) {
    load_file("quoted.csv");
    EXPECT_EQ(shown("B2"), "=1+1");
    EXPECT_EQ(sheet.formula_text(at("B2")), "");
}

TEST_F(CsvTest, ApostropheIsKept) {
    load_file("quoted.csv");
    EXPECT_EQ(shown("C2"), "'quoted");
}

TEST_F(CsvTest, LineBreakInsideQuotes) {
    load_file("multiline.csv");
    EXPECT_EQ(shown("B2"), "first line\nsecond line");
    // запись из двух строк файла всё равно одна строка таблицы
    EXPECT_EQ(shown("A3"), "2");
    EXPECT_EQ(shown("B3"), "short");
}

TEST_F(CsvTest, RowsOfDifferentLength) {
    load("1\n2,3,4\n\n5,6\n");
    EXPECT_EQ(sheet.used_range(), Range(at("A1"), at("C4")));
    EXPECT_EQ(shown("C2"), "4");
    EXPECT_EQ(sheet.raw(at("A3")), "");
    EXPECT_EQ(shown("B4"), "6");
    EXPECT_EQ(sheet.raw(at("B1")), "");
}

TEST_F(CsvTest, EmptyFile) {
    load_file("empty.csv");
    EXPECT_FALSE(sheet.used_range().has_value());
}

TEST_F(CsvTest, LastLineWithoutLineBreak) {
    load("a,b\nc,d");
    EXPECT_EQ(shown("B2"), "d");
}

TEST_F(CsvTest, EmptyFieldsAreSkipped) {
    load(",,x\n");
    EXPECT_EQ(sheet.used_range(), Range(at("C1"), at("C1")));
}

TEST_F(CsvTest, Utf8WithBom) {
    load("\xEF\xBB\xBF" "имя,1\n");
    EXPECT_EQ(shown("A1"), "имя");
    EXPECT_EQ(shown("B1"), "1");
}

TEST_F(CsvTest, UnclosedQuoteFile) {
    try {
        load_file("broken_quote.csv");
        FAIL() << "ждали FileFormatError";
    } catch (const FileFormatError& error) {
        EXPECT_EQ(error.line(), 2U);
    }
}

TEST_F(CsvTest, TextAfterClosingQuote) {
    try {
        load("a\n\"b\"c\n");
        FAIL() << "ждали FileFormatError";
    } catch (const FileFormatError& error) {
        EXPECT_EQ(error.line(), 2U);
    }
}

TEST_F(CsvTest, TooManyColumns) {
    const std::string row(static_cast<std::size_t>(ss::core::max_cols), ',');
    EXPECT_THROW(load(row + "x\n"), FileFormatError);
}
