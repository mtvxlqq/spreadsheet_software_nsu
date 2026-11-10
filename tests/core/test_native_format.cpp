#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/native_format.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"

using ss::core::Align;
using ss::core::CellAddress;
using ss::core::CellStyle;
using ss::core::FileFormatError;
using ss::core::NativeFormat;
using ss::core::Spreadsheet;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class NativeFormatTest : public ::testing::Test {
protected:
    NativeFormatTest() {
        ss::core::register_builtins(functions);
    }

    std::string save(const Spreadsheet& sheet) const {
        std::ostringstream out;
        format.save(out, sheet);
        return out.str();
    }

    void load(std::string_view text, Spreadsheet& sheet) const {
        std::istringstream in{std::string(text)};
        format.load(in, sheet);
    }

    void load_file(std::string_view name, Spreadsheet& sheet) const {
        std::ifstream in(std::string(SS_TESTDATA_DIR) + "/tables/" + std::string(name));
        ASSERT_TRUE(in.is_open()) << name;
        format.load(in, sheet);
    }

    // номер строки из ошибки, или 0, если ошибки не было
    std::size_t error_line(std::string_view text) {
        Spreadsheet sheet{functions};
        try {
            load(text, sheet);
        } catch (const FileFormatError& error) {
            return error.line();
        }
        return 0;
    }

    static std::string shown(const Spreadsheet& sheet, std::string_view address) {
        return sheet.value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    NativeFormat format;
};

}  // namespace

TEST_F(NativeFormatTest, SavesInRowOrder) {
    Spreadsheet sheet{functions};
    sheet.set_input(at("B2"), "=A1*2");
    sheet.set_input(at("A1"), "42");
    sheet.set_input(at("C1"), "text");
    EXPECT_EQ(save(sheet), "SST 2\nA1\t42\nC1\ttext\nB2\t=A1*2\n");
}

TEST_F(NativeFormatTest, EmptySheetIsJustSignature) {
    const Spreadsheet sheet{functions};
    EXPECT_EQ(save(sheet), "SST 2\n");
}

TEST_F(NativeFormatTest, EscapesSpecialCharacters) {
    Spreadsheet sheet{functions};
    sheet.set_input(at("A1"), "a\tb\\c\nd");
    EXPECT_EQ(save(sheet), "SST 2\nA1\ta\\tb\\\\c\\nd\n");
}

TEST_F(NativeFormatTest, RoundTripKeepsRawAndValues) {
    Spreadsheet original{functions};
    original.set_input(at("A1"), "42");
    original.set_input(at("A2"), "=A1*2");
    original.set_input(at("B1"), "tab\there, slash \\ and\nnew line");
    original.set_input(at("B2"), "привет, мир");
    original.set_input(at("C1"), "'123");
    original.set_input(at("C2"), "=SUM(A1:A2)");
    original.set_input(at("D1"), "=D2");
    original.set_input(at("D2"), "=D1");
    original.set_input(at("ZZ65536"), "last");

    Spreadsheet loaded{functions};
    load(save(original), loaded);

    original.for_each_cell([&](CellAddress address) {
        EXPECT_EQ(loaded.raw(address), original.raw(address)) << address.to_string();
        EXPECT_EQ(loaded.value(address)->to_text(), original.value(address)->to_text())
            << address.to_string();
    });
    EXPECT_EQ(loaded.used_range(), original.used_range());
    EXPECT_EQ(save(loaded), save(original));
}

TEST_F(NativeFormatTest, LoadsSimpleFile) {
    Spreadsheet sheet{functions};
    load_file("simple.sst", sheet);
    EXPECT_EQ(shown(sheet, "A1"), "42");
    EXPECT_EQ(shown(sheet, "B1"), "hello world");
    EXPECT_EQ(shown(sheet, "A2"), "84");
    EXPECT_EQ(shown(sheet, "C3"), "123");
    EXPECT_EQ(sheet.raw(at("C3")), "'123");
}

TEST_F(NativeFormatTest, ChainWrittenBackwardsIsCalculated) {
    Spreadsheet sheet{functions};
    load_file("chain.sst", sheet);
    EXPECT_EQ(shown(sheet, "A3"), "3");
}

TEST_F(NativeFormatTest, CycleFromFileBecomesError) {
    Spreadsheet sheet{functions};
    load_file("cycle.sst", sheet);
    EXPECT_EQ(shown(sheet, "A1"), "#CYCLE!");
    EXPECT_EQ(shown(sheet, "B1"), "#CYCLE!");
    EXPECT_EQ(shown(sheet, "C1"), "#CYCLE!");
    EXPECT_EQ(shown(sheet, "D1"), "5");
}

TEST_F(NativeFormatTest, BrokenSignatureFile) {
    Spreadsheet sheet{functions};
    try {
        load_file("broken_signature.sst", sheet);
        FAIL() << "ждали FileFormatError";
    } catch (const FileFormatError& error) {
        EXPECT_EQ(error.line(), 1U);
    }
    EXPECT_FALSE(sheet.used_range().has_value());
}

TEST_F(NativeFormatTest, BrokenEscapeFile) {
    Spreadsheet sheet{functions};
    try {
        load_file("broken_escape.sst", sheet);
        FAIL() << "ждали FileFormatError";
    } catch (const FileFormatError& error) {
        // пустая строка 3 пропускается, но в нумерации участвует
        EXPECT_EQ(error.line(), 4U);
    }
}

TEST_F(NativeFormatTest, BrokenFilesReportTheirLine) {
    EXPECT_EQ(error_line(""), 1U);
    EXPECT_EQ(error_line("SST 99\n"), 1U);                 // версия из будущего
    EXPECT_EQ(error_line("SST 3\n"), 1U);
    EXPECT_EQ(error_line("sst 1\n"), 1U);
    EXPECT_EQ(error_line("SST 1\nA1 42\n"), 2U);           // пробел вместо таба
    EXPECT_EQ(error_line("SST 1\nA1\t1\nA0\t2\n"), 3U);    // кривой адрес
    EXPECT_EQ(error_line("SST 1\nA1\t1\nAAA1\t2\n"), 3U);  // вне листа
    EXPECT_EQ(error_line("SST 1\nA1\t=1+\n"), 2U);         // битая формула
    EXPECT_EQ(error_line("SST 1\nA1\tend\\\n"), 2U);       // \ в самом конце
}

TEST_F(NativeFormatTest, WindowsLineEndingsAndEmptyLinesAreFine) {
    Spreadsheet sheet{functions};
    load("SST 1\r\nA1\t1\r\n\r\nA2\t=A1+1\r\n", sheet);
    EXPECT_EQ(shown(sheet, "A2"), "2");
    EXPECT_EQ(sheet.raw(at("A1")), "1");
}

TEST_F(NativeFormatTest, FormulaOnUnknownFunctionLoads) {
    // неизвестная функция это не ошибка файла, а #NAME? в ячейке
    Spreadsheet sheet{functions};
    load("SST 1\nA1\t=NOPE(1)\n", sheet);
    EXPECT_EQ(shown(sheet, "A1"), "#NAME?");
}

TEST_F(NativeFormatTest, SavesStylesAndWidthsAfterCells) {
    Spreadsheet sheet{functions};
    sheet.set_input(at("A1"), "42");
    sheet.set_input(at("B2"), "text");
    sheet.set_style(at("A1"), {.format = "fixed:2", .align = Align::Right});
    sheet.set_style(at("B2"), {.format = "", .align = Align::Center});
    sheet.set_style(at("C3"), {.format = "percent:0", .align = std::nullopt});
    sheet.set_column_width(1, 15);
    sheet.set_column_width(27, 4);
    EXPECT_EQ(save(sheet), "SST 2\n"
                           "A1\t42\n"
                           "B2\ttext\n"
                           "!style\tA1\tfixed:2\tright\n"
                           "!style\tB2\tgeneral\tcenter\n"
                           "!style\tC3\tpercent:0\tauto\n"
                           "!width\tB\t15\n"
                           "!width\tAB\t4\n");
}

TEST_F(NativeFormatTest, RoundTripKeepsStylesAndWidths) {
    Spreadsheet original{functions};
    original.set_input(at("A1"), "0.256");
    original.set_input(at("A2"), "=A1*2");
    original.set_style(at("A1"), {.format = "percent:1", .align = Align::Left});
    original.set_style(at("A2"), {.format = "fixed:3", .align = std::nullopt});
    // стиль пустой ячейки тоже должен сохраниться
    original.set_style(at("D7"), {.format = "", .align = Align::Right});
    original.set_column_width(0, 20);
    original.set_column_width(3, 3);

    Spreadsheet loaded{functions};
    load(save(original), loaded);

    EXPECT_EQ(loaded.style(at("A1")), original.style(at("A1")));
    EXPECT_EQ(loaded.style(at("A2")), original.style(at("A2")));
    EXPECT_EQ(loaded.style(at("D7")), original.style(at("D7")));
    EXPECT_EQ(loaded.style(at("B1")), CellStyle{});
    EXPECT_EQ(loaded.column_widths(), original.column_widths());
    EXPECT_EQ(loaded.raw(at("A2")), "=A1*2");
    // стиль пустой ячейки не делает её заполненной
    EXPECT_EQ(loaded.raw(at("D7")), "");
    EXPECT_EQ(loaded.used_range(), original.used_range());
    EXPECT_EQ(save(loaded), save(original));
}

TEST_F(NativeFormatTest, LoadsStyledFile) {
    Spreadsheet sheet{functions};
    load_file("styled.sst", sheet);
    EXPECT_EQ(shown(sheet, "B2"), "0.2");
    EXPECT_EQ(sheet.style(at("B2")), (CellStyle{.format = "percent:0", .align = std::nullopt}));
    EXPECT_EQ(sheet.style(at("A1")), (CellStyle{.format = "", .align = Align::Center}));
    EXPECT_EQ(sheet.style(at("B4")), (CellStyle{.format = "fixed:2", .align = Align::Right}));
    EXPECT_EQ(sheet.column_widths().get(0), 14);
    EXPECT_EQ(sheet.column_widths().get(1), 8);
    EXPECT_EQ(sheet.column_widths().get(2), ss::core::default_column_width);
}

// файлы, сохранённые до появления оформления, открываются как раньше
TEST_F(NativeFormatTest, OldVersionIsStillRead) {
    Spreadsheet sheet{functions};
    load("SST 1\nA1\t42\nA2\t=A1*2\n", sheet);
    EXPECT_EQ(shown(sheet, "A2"), "84");
    EXPECT_EQ(sheet.style(at("A1")), CellStyle{});
    EXPECT_TRUE(sheet.column_widths().custom().empty());
    // а сохраняется уже в новой версии
    EXPECT_EQ(save(sheet), "SST 2\nA1\t42\nA2\t=A1*2\n");
}

TEST_F(NativeFormatTest, OldVersionHasNoStyleLines) {
    EXPECT_EQ(error_line("SST 1\nA1\t1\n!style\tA1\tfixed:2\tright\n"), 3U);
    EXPECT_EQ(error_line("SST 1\n!width\tA\t15\n"), 2U);
}

// формат мог записать кто-то с другим набором форматов: не проверяем и не теряем
TEST_F(NativeFormatTest, UnknownNumberFormatIsKept) {
    Spreadsheet sheet{functions};
    load("SST 2\nA1\t0.5\n!style\tA1\tmoney:2\tauto\n", sheet);
    EXPECT_EQ(sheet.style(at("A1")).format, "money:2");
    EXPECT_EQ(save(sheet), "SST 2\nA1\t0.5\n!style\tA1\tmoney:2\tauto\n");
}

TEST_F(NativeFormatTest, StyleLinesMayComeBeforeCells) {
    Spreadsheet sheet{functions};
    load("SST 2\n!width\tA\t20\n!style\tA1\tfixed:1\tleft\nA1\t2.5\n", sheet);
    EXPECT_EQ(sheet.style(at("A1")), (CellStyle{.format = "fixed:1", .align = Align::Left}));
    EXPECT_EQ(sheet.column_widths().get(0), 20);
    EXPECT_EQ(shown(sheet, "A1"), "2.5");
}

TEST_F(NativeFormatTest, BrokenStyleLinesReportTheirLine) {
    EXPECT_EQ(error_line("SST 2\nA1\t1\n!style\tA1\tfixed:2\n"), 3U);             // нет выравнивания
    EXPECT_EQ(error_line("SST 2\n!style\tA1\tfixed:2\tright\textra\n"), 2U);      // лишнее поле
    EXPECT_EQ(error_line("SST 2\n!style\tA0\tfixed:2\tright\n"), 2U);             // кривой адрес
    EXPECT_EQ(error_line("SST 2\n!style\tAAA1\tfixed:2\tright\n"), 2U);           // вне листа
    EXPECT_EQ(error_line("SST 2\n!style\tA1\tfixed:2\tmiddle\n"), 2U);            // такого выравнивания нет
    EXPECT_EQ(error_line("SST 2\n!style\tA1\tfixed\\x\tright\n"), 2U);           // битый escape
    EXPECT_EQ(error_line("SST 2\n!style A1 fixed:2 right\n"), 2U);                 // пробелы вместо табов
    EXPECT_EQ(error_line("SST 2\n\n!color\tA1\tred\n"), 3U);                       // неизвестная строка
}

TEST_F(NativeFormatTest, BrokenWidthLinesReportTheirLine) {
    EXPECT_EQ(error_line("SST 2\n!width\tB\n"), 2U);            // нет ширины
    EXPECT_EQ(error_line("SST 2\n!width\tB\t15\t3\n"), 2U);    // лишнее поле
    EXPECT_EQ(error_line("SST 2\n!width\tB\twide\n"), 2U);     // не число
    EXPECT_EQ(error_line("SST 2\n!width\tB\t15.5\n"), 2U);
    EXPECT_EQ(error_line("SST 2\n!width\tB\t2\n"), 2U);        // меньше минимума
    EXPECT_EQ(error_line("SST 2\n!width\tB\t61\n"), 2U);       // больше максимума
    EXPECT_EQ(error_line("SST 2\n!width\tB1\t15\n"), 2U);      // адрес вместо столбца
    EXPECT_EQ(error_line("SST 2\n!width\t\t15\n"), 2U);        // пустое имя столбца
    EXPECT_EQ(error_line("SST 2\n!width\tAAA\t15\n"), 2U);     // столбец вне листа
    EXPECT_EQ(error_line("SST 2\nA1\t1\n!width\tB\t15\n"), 0U);
}

TEST_F(NativeFormatTest, BrokenStyleFile) {
    Spreadsheet sheet{functions};
    try {
        load_file("broken_style.sst", sheet);
        FAIL() << "ждали FileFormatError";
    } catch (const FileFormatError& error) {
        EXPECT_EQ(error.line(), 4U);
    }
}
