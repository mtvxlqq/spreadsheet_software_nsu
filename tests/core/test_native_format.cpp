#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/native_format.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"

using ss::core::CellAddress;
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
    EXPECT_EQ(save(sheet), "SST 1\nA1\t42\nC1\ttext\nB2\t=A1*2\n");
}

TEST_F(NativeFormatTest, EmptySheetIsJustSignature) {
    const Spreadsheet sheet{functions};
    EXPECT_EQ(save(sheet), "SST 1\n");
}

TEST_F(NativeFormatTest, EscapesSpecialCharacters) {
    Spreadsheet sheet{functions};
    sheet.set_input(at("A1"), "a\tb\\c\nd");
    EXPECT_EQ(save(sheet), "SST 1\nA1\ta\\tb\\\\c\\nd\n");
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
    EXPECT_EQ(error_line("SST 2\n"), 1U);
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
