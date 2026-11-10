#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/csv_reader.hpp"
#include "core/io/csv_writer.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "core/value_parser.hpp"
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
        ss::core::register_value_parsers(value_parsers);
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

    void set(std::string_view address, std::string_view raw) {
        sheet.set_input(at(address), raw);
    }

    // что CsvWriter запишет для текущего листа
    std::string exported() const {
        std::ostringstream out;
        ss::core::CsvWriter().save(out, sheet);
        return out.str();
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    Spreadsheet sheet{functions, value_parsers};
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

TEST_F(CsvTest, ExportWritesValuesRowByRow) {
    set("A1", "name");
    set("B1", "value");
    set("A2", "x");
    set("B2", "1.5");
    EXPECT_EQ(exported(), "name,value\r\nx,1.5\r\n");
}

TEST_F(CsvTest, ExportWritesResultsInsteadOfFormulas) {
    set("A1", "2");
    set("B1", "=A1*21");
    set("C1", "=SUM(A1:B1)");
    EXPECT_EQ(exported(), "2,42,44\r\n");
}

TEST_F(CsvTest, ExportWritesErrorsAsText) {
    set("A1", "=1/0");
    set("B1", "=NOPE(1)");
    set("C1", "=C1");
    EXPECT_EQ(exported(), "#DIV/0!,#NAME?,#CYCLE!\r\n");
}

TEST_F(CsvTest, ExportQuotesSpecialCharacters) {
    set("A1", "a,b");
    set("B1", "say \"hi\"");
    set("C1", "two\nlines");
    set("D1", "plain text");
    EXPECT_EQ(exported(), "\"a,b\",\"say \"\"hi\"\"\",\"two\nlines\",plain text\r\n");
}

TEST_F(CsvTest, ExportStartsAtA1AndKeepsEmptyCells) {
    set("B2", "x");
    set("D3", "y");
    // три строки по четыре поля, пустые ячейки это пустые поля
    EXPECT_EQ(exported(), ",,,\r\n,x,,\r\n,,,y\r\n");
}

TEST_F(CsvTest, ExportOfEmptySheetIsEmptyFile) {
    EXPECT_EQ(exported(), "");
}

TEST_F(CsvTest, ExportWritesDatesAndFullPrecision) {
    set("A1", "15.03.2024");
    set("B1", "=A1+1");
    set("C1", "=1/3");
    // оформление в CSV не попадает, число пишется как есть
    sheet.set_style(at("C1"), {.format = "fixed:2", .align = std::nullopt});
    EXPECT_EQ(exported(), "2024-03-15,2024-03-16,0.3333333333\r\n");
}

TEST_F(CsvTest, ExportedFileImportsBackWithSameValues) {
    set("A1", "текст, с запятой");
    set("B1", "=2*3");
    set("C1", "15.03.2024");
    set("A2", "say \"hi\"");
    set("C2", "multi\nline");
    set("B3", "=1/0");
    set("D3", "\'=not a formula");

    Spreadsheet loaded{functions, value_parsers};
    std::istringstream in(exported());
    reader.load(in, loaded);

    sheet.for_each_cell([&](CellAddress address) {
        EXPECT_EQ(loaded.value(address)->to_text(), sheet.value(address)->to_text())
            << address.to_string();
    });
    EXPECT_EQ(loaded.used_range(), sheet.used_range());
    // формул в файле не было, только их результаты
    EXPECT_EQ(loaded.raw(at("B1")), "6");
    EXPECT_TRUE(loaded.value(at("C1"))->as_date().has_value());
    // текст, похожий на формулу, формулой не стал
    EXPECT_EQ(loaded.formula_text(at("D3")), "");
}

TEST_F(CsvTest, WriterIsForExportOnly) {
    const ss::core::CsvWriter writer;
    EXPECT_EQ(writer.name(), "CSV");
    EXPECT_EQ(writer.extensions(), std::vector<std::string>{".csv"});
    EXPECT_FALSE(writer.keeps_formulas());
}
