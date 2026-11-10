#include <chrono>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_style.hpp"
#include "core/column_widths.hpp"
#include "core/document_edit.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/native_format.hpp"
#include "core/ref_mapping.hpp"
#include "core/spreadsheet.hpp"
#include "core/structure_edit.hpp"
#include "core/value.hpp"
#include "support/printers.hpp"

using ss::core::Align;
using ss::core::Axis;
using ss::core::CellAddress;
using ss::core::CellStyle;
using ss::core::DocumentEdit;
using ss::core::Spreadsheet;
using ss::core::StructureEdit;

namespace {

using Kind = StructureEdit::Kind;

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

class StructureEditTest : public ::testing::Test {
protected:
    StructureEditTest() {
        ss::core::register_builtins(functions);
    }

    void set(std::string_view address, std::string_view raw) {
        sheet.set_input(at(address), raw);
    }

    // номера строк и столбцов тут как на экране, с единицы
    DocumentEdit change(Kind kind, Axis axis, int number, int count = 1) {
        DocumentEdit edit = ss::core::plan_structure_edit(
            sheet, functions, {.kind = kind, .axis = axis, .at = number - 1, .count = count});
        sheet.apply(edit);
        return edit;
    }

    DocumentEdit insert_rows(int row, int count = 1) {
        return change(Kind::Insert, Axis::Row, row, count);
    }

    DocumentEdit delete_rows(int row, int count = 1) {
        return change(Kind::Delete, Axis::Row, row, count);
    }

    DocumentEdit insert_cols(int col, int count = 1) {
        return change(Kind::Insert, Axis::Column, col, count);
    }

    DocumentEdit delete_cols(int col, int count = 1) {
        return change(Kind::Delete, Axis::Column, col, count);
    }

    std::string raw(std::string_view address) const {
        return sheet.raw(at(address));
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    Spreadsheet sheet{functions};
};

}  // namespace

TEST_F(StructureEditTest, InsertRowInTheMiddleMovesCellsDown) {
    set("A1", "one");
    set("A2", "two");
    set("A3", "three");
    insert_rows(2);
    EXPECT_EQ(raw("A1"), "one");
    EXPECT_EQ(raw("A2"), "");
    EXPECT_EQ(raw("A3"), "two");
    EXPECT_EQ(raw("A4"), "three");
}

TEST_F(StructureEditTest, InsertRowAtTheTop) {
    set("A1", "one");
    set("B1", "=A1");
    insert_rows(1, 2);
    EXPECT_EQ(raw("A1"), "");
    EXPECT_EQ(raw("A3"), "one");
    EXPECT_EQ(raw("B3"), "=A3");
}

TEST_F(StructureEditTest, InsertRowBelowEverythingChangesNothing) {
    set("A1", "one");
    set("A2", "=A1");
    const DocumentEdit edit = insert_rows(10);
    EXPECT_TRUE(edit.cells.empty());
    EXPECT_EQ(raw("A2"), "=A1");
}

// формулы меняются во всём листе, а не только в сдвинутых ячейках
TEST_F(StructureEditTest, InsertRowRewritesFormulasAboveIt) {
    set("A1", "=A10+1");
    set("B1", "=SUM(A5:A10)");
    set("A10", "7");
    insert_rows(5);
    EXPECT_EQ(raw("A1"), "=A11+1");
    EXPECT_EQ(raw("B1"), "=SUM(A6:A11)");
    EXPECT_EQ(raw("A11"), "7");
    EXPECT_EQ(shown("A1"), "8");
    EXPECT_EQ(shown("B1"), "7");
}

// $ важен только при копировании, вставка строки двигает и такие ссылки
TEST_F(StructureEditTest, AbsoluteReferencesMoveToo) {
    set("A5", "3");
    set("B1", "=$A$5+A$5+$A5");
    insert_rows(2);
    EXPECT_EQ(raw("B1"), "=$A$6+A$6+$A6");
    EXPECT_EQ(shown("B1"), "9");
}

TEST_F(StructureEditTest, InsertInsideRangeStretchesIt) {
    set("A1", "1");
    set("A2", "2");
    set("A3", "3");
    set("B1", "=SUM(A1:A3)");
    insert_rows(2);
    EXPECT_EQ(raw("B1"), "=SUM(A1:A4)");
    EXPECT_EQ(shown("B1"), "6");
    // в новую строку можно писать, и сумма её учтёт
    set("A2", "10");
    EXPECT_EQ(shown("B1"), "16");
}

TEST_F(StructureEditTest, FormulaWithoutMovedReferencesKeepsItsSpelling) {
    set("A1", "= a1 + sum( b1 : b2 )");
    insert_rows(5);
    EXPECT_EQ(raw("A1"), "= a1 + sum( b1 : b2 )");
}

TEST_F(StructureEditTest, DeleteRowMovesCellsUp) {
    set("A1", "one");
    set("A2", "two");
    set("A3", "three");
    set("A4", "four");
    delete_rows(2, 2);
    EXPECT_EQ(raw("A1"), "one");
    EXPECT_EQ(raw("A2"), "four");
    EXPECT_EQ(raw("A3"), "");
    EXPECT_EQ(raw("A4"), "");
}

TEST_F(StructureEditTest, DeleteFirstAndLastRows) {
    set("A1", "one");
    set("A2", "two");
    delete_rows(1);
    EXPECT_EQ(raw("A1"), "two");
    set("A65536", "last");
    delete_rows(65536);
    EXPECT_EQ(raw("A65536"), "");
    // лишнее количество за краем листа просто обрезается
    EXPECT_NO_THROW(delete_rows(65530, 100));
}

TEST_F(StructureEditTest, ReferenceToDeletedRowBecomesRefError) {
    set("A1", "=A3+1");
    set("B1", "=SUM(A3:A3)");
    set("A3", "5");
    delete_rows(3);
    EXPECT_EQ(raw("A1"), "=#REF!+1");
    EXPECT_EQ(shown("A1"), "#REF!");
    EXPECT_EQ(raw("B1"), "=SUM(#REF!)");
    EXPECT_EQ(shown("B1"), "#REF!");
}

TEST_F(StructureEditTest, ReferenceBelowDeletedRowMovesUp) {
    set("A1", "=A5*2");
    set("A5", "4");
    delete_rows(2, 2);
    EXPECT_EQ(raw("A1"), "=A3*2");
    EXPECT_EQ(shown("A1"), "8");
}

TEST_F(StructureEditTest, DeletingEdgeOfRangeShrinksIt) {
    set("A1", "1");
    set("A2", "2");
    set("A3", "3");
    set("B1", "=SUM(A1:A3)");
    set("C1", "=SUM(A2:A3)");
    delete_rows(3);
    EXPECT_EQ(raw("B1"), "=SUM(A1:A2)");
    EXPECT_EQ(shown("B1"), "3");
    EXPECT_EQ(raw("C1"), "=SUM(A2:A2)");
    delete_rows(2);
    EXPECT_EQ(raw("C1"), "=SUM(#REF!)");
}

// #REF! остаётся в формуле текстом и переживает сохранение и открытие
TEST_F(StructureEditTest, RefErrorIsSavedToFile) {
    set("A1", "=A2+1");
    set("A2", "5");
    delete_rows(2);

    const ss::core::NativeFormat format;
    std::ostringstream out;
    format.save(out, sheet);
    EXPECT_EQ(out.str(), "SST 2\nA1\t=#REF!+1\n");

    Spreadsheet loaded{functions};
    std::istringstream in(out.str());
    format.load(in, loaded);
    EXPECT_EQ(loaded.raw(at("A1")), "=#REF!+1");
    EXPECT_EQ(loaded.value(at("A1"))->to_text(), "#REF!");
}

TEST_F(StructureEditTest, InverseRestoresDeletedCellsAndFormulas) {
    set("A1", "=A2+A3");
    set("A2", "2");
    set("A3", "=A2*10");
    set("B3", "keep");
    const DocumentEdit edit = delete_rows(2);
    EXPECT_EQ(raw("A1"), "=#REF!+A2");

    sheet.apply(edit.inverse());
    EXPECT_EQ(raw("A1"), "=A2+A3");
    EXPECT_EQ(raw("A2"), "2");
    EXPECT_EQ(raw("A3"), "=A2*10");
    EXPECT_EQ(raw("B3"), "keep");
    EXPECT_EQ(shown("A1"), "22");
}

TEST_F(StructureEditTest, InverseOfInsertRestoresSpelling) {
    set("A1", "= a5 * 2");
    set("A5", "3");
    const DocumentEdit edit = insert_rows(3);
    EXPECT_EQ(raw("A1"), "=A6*2");
    sheet.apply(edit.inverse());
    EXPECT_EQ(raw("A1"), "= a5 * 2");
    EXPECT_EQ(raw("A5"), "3");
    EXPECT_EQ(raw("A6"), "");
}

TEST_F(StructureEditTest, ValuesAreRecalculatedAfterInsert) {
    set("A1", "1");
    set("A2", "=A1+1");
    set("A3", "=A2+1");
    insert_rows(2);
    EXPECT_EQ(raw("A4"), "=A3+1");
    EXPECT_EQ(shown("A3"), "2");
    EXPECT_EQ(shown("A4"), "3");
    set("A1", "10");
    EXPECT_EQ(shown("A4"), "12");
}

TEST_F(StructureEditTest, CycleSurvivesInsertAndDelete) {
    set("A1", "=A2");
    set("A2", "=A1");
    set("B1", "=A1+1");
    insert_rows(1);
    EXPECT_EQ(raw("A2"), "=A3");
    EXPECT_EQ(raw("A3"), "=A2");
    EXPECT_EQ(shown("A2"), "#CYCLE!");
    EXPECT_EQ(shown("B2"), "#CYCLE!");
    // удалили одну из ячеек цикла: цикла больше нет, осталась ссылка в никуда
    delete_rows(3);
    EXPECT_EQ(raw("A2"), "=#REF!");
    EXPECT_EQ(shown("A2"), "#REF!");
    EXPECT_EQ(shown("B2"), "#REF!");
}

TEST_F(StructureEditTest, InsertColumnMovesCellsAndFormulas) {
    set("A1", "1");
    set("B1", "2");
    set("C1", "=A1+B1");
    set("A2", "=SUM(A1:C1)");
    insert_cols(2);
    EXPECT_EQ(raw("A1"), "1");
    EXPECT_EQ(raw("B1"), "");
    EXPECT_EQ(raw("C1"), "2");
    EXPECT_EQ(raw("D1"), "=A1+C1");
    EXPECT_EQ(raw("A2"), "=SUM(A1:D1)");
    EXPECT_EQ(shown("D1"), "3");
    EXPECT_EQ(shown("A2"), "6");
}

TEST_F(StructureEditTest, DeleteColumnGivesRefError) {
    set("A1", "1");
    set("B1", "2");
    set("C1", "=A1+B1");
    set("D1", "=C1*2");
    delete_cols(2);
    EXPECT_EQ(raw("B1"), "=A1+#REF!");
    EXPECT_EQ(raw("C1"), "=B1*2");
    EXPECT_EQ(shown("C1"), "#REF!");
    EXPECT_EQ(raw("D1"), "");
}

TEST_F(StructureEditTest, StylesMoveWithCells) {
    const CellStyle money{.format = "fixed:2", .align = Align::Right};
    const CellStyle centered{.format = "", .align = Align::Center};
    set("A2", "1.5");
    sheet.set_style(at("A2"), money);
    // стиль пустой ячейки тоже переезжает
    sheet.set_style(at("C4"), centered);

    insert_rows(2);
    EXPECT_EQ(sheet.style(at("A3")), money);
    EXPECT_EQ(sheet.style(at("A2")), CellStyle{});
    EXPECT_EQ(sheet.style(at("C5")), centered);
    EXPECT_EQ(sheet.style(at("C4")), CellStyle{});

    insert_cols(1);
    EXPECT_EQ(sheet.style(at("B3")), money);
    EXPECT_EQ(sheet.style(at("D5")), centered);

    delete_rows(3);
    EXPECT_EQ(sheet.style(at("B3")), CellStyle{});
    EXPECT_EQ(sheet.style(at("D4")), centered);
}

TEST_F(StructureEditTest, ColumnWidthsMoveWithColumns) {
    sheet.set_column_width(1, 20);
    sheet.set_column_width(3, 5);
    const DocumentEdit inserted = insert_cols(2, 2);
    EXPECT_EQ(sheet.column_widths().get(1), 10);
    EXPECT_EQ(sheet.column_widths().get(3), 20);
    EXPECT_EQ(sheet.column_widths().get(5), 5);

    const DocumentEdit deleted = delete_cols(4);
    EXPECT_EQ(sheet.column_widths().get(3), 10);
    EXPECT_EQ(sheet.column_widths().get(4), 5);

    sheet.apply(deleted.inverse());
    sheet.apply(inserted.inverse());
    EXPECT_EQ(sheet.column_widths().get(1), 20);
    EXPECT_EQ(sheet.column_widths().get(3), 5);
    EXPECT_EQ(sheet.column_widths().custom().size(), 2U);
}

TEST_F(StructureEditTest, RowsDoNotTouchColumnWidths) {
    sheet.set_column_width(1, 20);
    const DocumentEdit edit = insert_rows(1);
    EXPECT_FALSE(edit.widths_after.has_value());
    EXPECT_EQ(sheet.column_widths().get(1), 20);
}

// заполненная ячейка ушла бы за край листа: ничего не меняем
TEST_F(StructureEditTest, InsertThatPushesCellOffSheetIsRejected) {
    set("A65536", "last");
    set("A1", "=A65536");
    EXPECT_THROW(insert_rows(5), ss::core::LimitError);
    EXPECT_EQ(raw("A65536"), "last");
    EXPECT_EQ(raw("A1"), "=A65536");

    set("ZZ1", "edge");
    EXPECT_THROW(insert_cols(1), ss::core::LimitError);
    EXPECT_EQ(raw("ZZ1"), "edge");
    // перед самой последней строкой тоже нельзя, она занята
    EXPECT_THROW(insert_rows(65536), ss::core::LimitError);
    EXPECT_EQ(raw("A65536"), "last");
}

TEST_F(StructureEditTest, InsertNearSheetEdgeWorksWhileNothingFallsOff) {
    set("A65530", "near");
    set("A1", "=A65530");
    insert_rows(10, 6);
    EXPECT_EQ(raw("A65536"), "near");
    EXPECT_EQ(raw("A1"), "=A65536");
}

TEST_F(StructureEditTest, StyleOfEmptyCellMayFallOffSheet) {
    sheet.set_style(at("A65536"), {.format = "fixed:1", .align = std::nullopt});
    const DocumentEdit edit = insert_rows(1);
    EXPECT_EQ(sheet.style(at("A65536")), CellStyle{});
    sheet.apply(edit.inverse());
    EXPECT_EQ(sheet.style(at("A65536")).format, "fixed:1");
}

TEST_F(StructureEditTest, BadPositionAndCountAreRejected) {
    for (const StructureEdit bad : {
             StructureEdit{.kind = Kind::Insert, .axis = Axis::Row, .at = -1, .count = 1},
             StructureEdit{.kind = Kind::Insert, .axis = Axis::Row, .at = 65536, .count = 1},
             StructureEdit{.kind = Kind::Insert, .axis = Axis::Row, .at = 0, .count = 0},
             StructureEdit{.kind = Kind::Delete, .axis = Axis::Row, .at = 0, .count = -3},
             StructureEdit{.kind = Kind::Insert, .axis = Axis::Column, .at = 702, .count = 1},
             StructureEdit{.kind = Kind::Delete, .axis = Axis::Column, .at = 0, .count = 703},
         }) {
        EXPECT_THROW(ss::core::plan_structure_edit(sheet, functions, bad), ss::core::LimitError);
    }
}

// планирование таблицу не меняет
TEST_F(StructureEditTest, PlanDoesNotTouchSheet) {
    set("A1", "=A2");
    set("A2", "1");
    const DocumentEdit edit = ss::core::plan_structure_edit(
        sheet, functions, {.kind = Kind::Delete, .axis = Axis::Row, .at = 1, .count = 1});
    EXPECT_FALSE(edit.cells.empty());
    EXPECT_EQ(raw("A1"), "=A2");
    EXPECT_EQ(raw("A2"), "1");
}

// большой лист: вставка строки в начало двигает все 50 000 ячеек
TEST_F(StructureEditTest, BigSheetIsHandledQuickly) {
    constexpr int rows = 5000;
    constexpr int cols = 10;
    for (int row = 0; row < rows; ++row) {
        // в каждой строке свои числа, иначе после сдвига ячейки совпали бы
        // с соседями и менять было бы почти нечего
        const std::string step = std::to_string(row + 1);
        sheet.set_input({.row = row, .col = 0}, step);
        for (int col = 1; col < cols; ++col) {
            const CellAddress left{.row = row, .col = col - 1};
            sheet.set_input({.row = row, .col = col}, "=" + left.to_string() + "+" + step);
        }
    }
    EXPECT_EQ(shown("J5000"), "50000");

    const auto started = std::chrono::steady_clock::now();
    const DocumentEdit inserted = insert_rows(1);
    EXPECT_GE(inserted.cells.size(), 50000U);
    EXPECT_EQ(raw("J5001"), "=I5001+5000");
    EXPECT_EQ(shown("J5001"), "50000");
    EXPECT_EQ(raw("J1"), "");

    sheet.apply(inserted.inverse());
    EXPECT_EQ(raw("J5000"), "=I5000+5000");
    EXPECT_EQ(shown("J5000"), "50000");
    // запас большой: под санитайзером всё идёт раз в двадцать медленнее
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(60));
}
