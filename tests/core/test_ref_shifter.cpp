#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/errors.hpp"
#include "core/formula_printer.hpp"
#include "core/formula_rewrite.hpp"
#include "core/parser.hpp"
#include "core/ref_mapping.hpp"
#include "core/ref_shifter.hpp"
#include "support/formula_fixture.hpp"

using ss::core::Axis;
using ss::core::CopyMapping;
using ss::core::FormulaPrinter;
using ss::core::max_cols;
using ss::core::max_rows;
using ss::core::Parser;
using ss::core::RefShifter;
using ss::core::rewrite_formula;
using ss::core::StructureEdit;
using ss::core::StructureMapping;
using ss::core::translate_formula;

namespace {

using Kind = StructureEdit::Kind;

class RefShifterTest : public ss::test::FormulaTest {
protected:
    std::string copied(std::string_view raw, int d_row, int d_col) {
        return translate_formula(raw, functions, d_row, d_col);
    }

    // номера строк и столбцов тут с нуля, как в CellAddress
    std::string rows(std::string_view raw, Kind kind, int at, int count = 1) {
        return rewrite_formula(raw, functions,
                               StructureMapping({.kind = kind, .axis = Axis::Row, .at = at, .count = count}));
    }

    std::string cols(std::string_view raw, Kind kind, int at, int count = 1) {
        return rewrite_formula(raw, functions,
                               StructureMapping({.kind = kind, .axis = Axis::Column, .at = at, .count = count}));
    }
};

}  // namespace

TEST_F(RefShifterTest, CopyMovesRelativeRef) {
    EXPECT_EQ(copied("=A1", 1, 1), "=B2");
    EXPECT_EQ(copied("=B2", -1, -1), "=A1");
}

TEST_F(RefShifterTest, CopyKeepsAbsoluteColumn) {
    EXPECT_EQ(copied("=$A1", 1, 1), "=$A2");
}

TEST_F(RefShifterTest, CopyKeepsAbsoluteRow) {
    EXPECT_EQ(copied("=A$1", 1, 1), "=B$1");
}

TEST_F(RefShifterTest, CopyKeepsFullyAbsolute) {
    EXPECT_EQ(copied("=$A$1", 5, 5), "=$A$1");
}

TEST_F(RefShifterTest, CopyMovesRange) {
    EXPECT_EQ(copied("=SUM(A1:B2)", 1, 0), "=SUM(A2:B3)");
}

TEST_F(RefShifterTest, CopyRangeWithMixedAnchors) {
    EXPECT_EQ(copied("=SUM($A1:B$2)", 1, 1), "=SUM($A2:C$2)");
}

TEST_F(RefShifterTest, CopyGoesInsideNestedExpressions) {
    EXPECT_EQ(copied("=IF(A1>0,SUM(A1:A3),-A2)*2", 1, 0), "=IF(A2>0,SUM(A2:A4),-A3)*2");
}

TEST_F(RefShifterTest, CopyKeepsParentheses) {
    EXPECT_EQ(copied("=(A1+B1)*2", 0, 1), "=(B1+C1)*2");
}

TEST_F(RefShifterTest, CopyOffSheetBecomesRefError) {
    EXPECT_EQ(copied("=A1", -1, 0), "=#REF!");
    EXPECT_EQ(copied("=A1", 0, -1), "=#REF!");
    EXPECT_EQ(copied("=ZZ1", 0, 1), "=#REF!");
    EXPECT_EQ(copied("=A65536", 1, 0), "=#REF!");
}

TEST_F(RefShifterTest, CopyRangePartlyOffSheetBecomesRefError) {
    EXPECT_EQ(copied("=SUM(A1:A3)", -1, 0), "=SUM(#REF!)");
}

TEST_F(RefShifterTest, CopyKeepsOnlyBrokenPart) {
    EXPECT_EQ(copied("=A1+B2", -1, 0), "=#REF!+B1");
}

TEST_F(RefShifterTest, CopyLeavesNonFormulasAlone) {
    EXPECT_EQ(copied("12", 1, 1), "12");
    EXPECT_EQ(copied("text", 1, 1), "text");
    EXPECT_EQ(copied("'=A1", 1, 1), "'=A1");
}

TEST_F(RefShifterTest, UnchangedFormulaKeepsSpelling) {
    EXPECT_EQ(copied("=sum( $a$1 )  ", 3, 3), "=sum( $a$1 )  ");
    EXPECT_EQ(copied("=1 + 2", 1, 1), "=1 + 2");
}

TEST_F(RefShifterTest, UnknownFunctionIsKept) {
    EXPECT_EQ(copied("=FOO(A1)", 1, 0), "=FOO(A2)");
}

TEST_F(RefShifterTest, BrokenFormulaThrows) {
    EXPECT_THROW(copied("=1+", 1, 0), ss::core::ParseError);
}

TEST_F(RefShifterTest, OriginalTreeIsNotChanged) {
    const auto formula = Parser::parse("=SUM(A1:A2)+B1", functions);
    const auto shifted = RefShifter::shift(*formula, CopyMapping(3, 0));
    EXPECT_EQ(FormulaPrinter::print(*formula), "=SUM(A1:A2)+B1");
    EXPECT_EQ(FormulaPrinter::print(*shifted), "=SUM(A4:A5)+B4");
}

TEST_F(RefShifterTest, InsertRowMovesRefBelow) {
    EXPECT_EQ(rows("=A5", Kind::Insert, 2), "=A6");
    EXPECT_EQ(rows("=A3", Kind::Insert, 2), "=A4");
}

TEST_F(RefShifterTest, InsertRowLeavesRefAbove) {
    EXPECT_EQ(rows("=A2", Kind::Insert, 2), "=A2");
}

TEST_F(RefShifterTest, InsertSeveralRows) {
    EXPECT_EQ(rows("=A5", Kind::Insert, 2, 3), "=A8");
}

TEST_F(RefShifterTest, InsertMovesAbsoluteRefsToo) {
    EXPECT_EQ(rows("=$A$5", Kind::Insert, 2), "=$A$6");
    EXPECT_EQ(cols("=$C$1", Kind::Insert, 1), "=$D$1");
}

TEST_F(RefShifterTest, InsertInsideRangeStretchesIt) {
    EXPECT_EQ(rows("=SUM(A1:A5)", Kind::Insert, 2), "=SUM(A1:A6)");
}

TEST_F(RefShifterTest, InsertBeforeRangeMovesItWhole) {
    EXPECT_EQ(rows("=SUM(A3:A5)", Kind::Insert, 2), "=SUM(A4:A6)");
}

TEST_F(RefShifterTest, InsertAfterRangeLeavesIt) {
    EXPECT_EQ(rows("=SUM(A1:A2)", Kind::Insert, 2), "=SUM(A1:A2)");
}

TEST_F(RefShifterTest, InsertPushingRefOffSheetIsRefError) {
    EXPECT_EQ(rows("=A65536", Kind::Insert, 0), "=#REF!");
}

TEST_F(RefShifterTest, InsertClampsRangeAtSheetEnd) {
    EXPECT_EQ(rows("=SUM(A1:A65536)", Kind::Insert, 2), "=SUM(A1:A65536)");
    EXPECT_EQ(rows("=SUM(A65535:A65536)", Kind::Insert, max_rows - 2), "=SUM(A65536:A65536)");
}

TEST_F(RefShifterTest, InsertPushingWholeRangeOffSheetIsRefError) {
    EXPECT_EQ(rows("=SUM(A65536:A65536)", Kind::Insert, max_rows - 1), "=SUM(#REF!)");
}

TEST_F(RefShifterTest, DeleteRowOfRefIsRefError) {
    EXPECT_EQ(rows("=A3", Kind::Delete, 2), "=#REF!");
    EXPECT_EQ(rows("=$A$3+1", Kind::Delete, 2), "=#REF!+1");
}

TEST_F(RefShifterTest, DeleteRowMovesRefBelow) {
    EXPECT_EQ(rows("=A5", Kind::Delete, 2), "=A4");
    EXPECT_EQ(rows("=A5", Kind::Delete, 1, 2), "=A3");
}

TEST_F(RefShifterTest, DeleteRowLeavesRefAbove) {
    EXPECT_EQ(rows("=A2", Kind::Delete, 2), "=A2");
}

TEST_F(RefShifterTest, DeleteInsideRangeShrinksIt) {
    EXPECT_EQ(rows("=SUM(A1:A5)", Kind::Delete, 1, 2), "=SUM(A1:A3)");
}

TEST_F(RefShifterTest, DeleteLastRowOfRangeShrinksIt) {
    EXPECT_EQ(rows("=SUM(A1:A5)", Kind::Delete, 4), "=SUM(A1:A4)");
}

TEST_F(RefShifterTest, DeleteFirstRowOfRangeShrinksIt) {
    EXPECT_EQ(rows("=SUM(A1:A5)", Kind::Delete, 0), "=SUM(A1:A4)");
    EXPECT_EQ(rows("=SUM(A2:A5)", Kind::Delete, 1), "=SUM(A2:A4)");
}

TEST_F(RefShifterTest, DeleteOverlappingRangeEnd) {
    EXPECT_EQ(rows("=SUM(A3:A5)", Kind::Delete, 3, 5), "=SUM(A3:A3)");
    EXPECT_EQ(rows("=SUM(A3:A5)", Kind::Delete, 0, 3), "=SUM(A1:A2)");
}

TEST_F(RefShifterTest, DeleteWholeRangeIsRefError) {
    EXPECT_EQ(rows("=SUM(A2:A3)", Kind::Delete, 1, 2), "=SUM(#REF!)");
}

TEST_F(RefShifterTest, DeleteBelowRangeLeavesIt) {
    EXPECT_EQ(rows("=SUM(A1:A2)", Kind::Delete, 2), "=SUM(A1:A2)");
}

TEST_F(RefShifterTest, ColumnsWorkLikeRows) {
    EXPECT_EQ(cols("=E1", Kind::Insert, 2), "=F1");
    EXPECT_EQ(cols("=B1", Kind::Insert, 2), "=B1");
    EXPECT_EQ(cols("=C1", Kind::Delete, 2), "=#REF!");
    EXPECT_EQ(cols("=E1", Kind::Delete, 2), "=D1");
    EXPECT_EQ(cols("=SUM(A1:E1)", Kind::Insert, 2), "=SUM(A1:F1)");
    EXPECT_EQ(cols("=SUM(A1:E1)", Kind::Delete, 1, 2), "=SUM(A1:C1)");
    EXPECT_EQ(cols("=SUM(B1:C1)", Kind::Delete, 1, 2), "=SUM(#REF!)");
}

TEST_F(RefShifterTest, ColumnPushedOffSheetIsRefError) {
    EXPECT_EQ(cols("=ZZ1", Kind::Insert, max_cols - 1), "=#REF!");
}

TEST_F(RefShifterTest, RowEditDoesNotTouchColumns) {
    EXPECT_EQ(rows("=C5+$D$9", Kind::Insert, 0), "=C6+$D$10");
}

TEST_F(RefShifterTest, ReversedRangeKeepsItsOrder) {
    EXPECT_EQ(rows("=SUM(A5:A1)", Kind::Insert, 2), "=SUM(A6:A1)");
    EXPECT_EQ(rows("=SUM(A5:A1)", Kind::Delete, 4), "=SUM(A4:A1)");
}

TEST_F(RefShifterTest, RefOutsideSheetIsLeftAlone) {
    EXPECT_EQ(rows("=A99999999", Kind::Insert, 0), "=A99999999");
}

TEST_F(RefShifterTest, ExistingRefErrorStays) {
    EXPECT_EQ(rows("=#REF!+A5", Kind::Insert, 0), "=#REF!+A6");
}

TEST_F(RefShifterTest, ShiftIfChangedTellsWhenNothingMoved) {
    const CopyMapping down(1, 0);
    const auto with_ref = Parser::parse("=A1+1", functions);
    const auto without_refs = Parser::parse("=1+SUM(2, 3)", functions);
    const auto absolute = Parser::parse("=$A$1*2", functions);

    const auto shifted = RefShifter::shift_if_changed(*with_ref, down);
    ASSERT_NE(shifted, nullptr);
    EXPECT_EQ(FormulaPrinter::print(*shifted), "=A2+1");
    EXPECT_EQ(RefShifter::shift_if_changed(*without_refs, down), nullptr);
    EXPECT_EQ(RefShifter::shift_if_changed(*absolute, down), nullptr);
    // обычный shift копию возвращает всегда
    EXPECT_EQ(FormulaPrinter::print(*RefShifter::shift(*absolute, down)), "=$A$1*2");
}

// ссылка глубоко в дереве тоже считается
TEST_F(RefShifterTest, ShiftIfChangedSeesNestedReferences) {
    const StructureMapping insert({.kind = Kind::Insert, .axis = Axis::Row, .at = 0, .count = 1});
    const auto nested = Parser::parse("=IF(1, -(2+SUM(3, B5:B6)), 0)", functions);
    const auto shifted = RefShifter::shift_if_changed(*nested, insert);
    ASSERT_NE(shifted, nullptr);
    EXPECT_EQ(FormulaPrinter::print(*shifted), "=IF(1,-(2+SUM(3,B6:B7)),0)");

    const StructureMapping below({.kind = Kind::Insert, .axis = Axis::Row, .at = 50, .count = 1});
    EXPECT_EQ(RefShifter::shift_if_changed(*nested, below), nullptr);
}
