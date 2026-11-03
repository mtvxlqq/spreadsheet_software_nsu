#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_io.hpp"
#include "core/cell_observer.hpp"
#include "core/errors.hpp"
#include "core/eval_context.hpp"
#include "core/function.hpp"
#include "core/function_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/value.hpp"
#include "support/printers.hpp"

using ss::core::CellAddress;
using ss::core::Spreadsheet;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

std::string join(std::span<const CellAddress> cells) {
    std::string result;
    for (const CellAddress address : cells) {
        result += (result.empty() ? "" : " ") + address.to_string();
    }
    return result;
}

// COUNTCALLS(x) возвращает x и считает, сколько раз его вызвали. Есть только
// в тестах: заодно видно, что функцию можно добавить без правки ядра.
class CountCallsFunction final : public ss::core::Function {
public:
    explicit CountCallsFunction(int& counter) : counter_(counter) {}

    std::string_view name() const override {
        return "COUNTCALLS";
    }

    ss::core::ValuePtr call(std::span<const ss::core::Expr* const> args,
                            ss::core::EvalContext& context) const override {
        ++counter_;
        if (args.size() != 1) {
            return ss::core::make_error(ss::core::ErrorCode::Value);
        }
        return context.evaluate(*args[0]);
    }

private:
    int& counter_;
};

// запоминает каждое уведомление строкой вида "A1 B1"
class RecordingObserver : public ss::core::CellObserver {
public:
    void on_cells_changed(std::span<const CellAddress> cells) override {
        calls.push_back(join(cells));
    }

    std::vector<std::string> calls;
};

class SpreadsheetTest : public ::testing::Test {
protected:
    SpreadsheetTest() {
        ss::core::register_builtins(functions);
        functions.add("COUNTCALLS",
                      [this] { return std::make_shared<CountCallsFunction>(count_calls); });
        sheet.subscribe(&observer);
    }

    ~SpreadsheetTest() override {
        sheet.unsubscribe(&observer);
    }

    void input(std::string_view address, std::string_view raw) {
        sheet.set_input(at(address), raw);
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    // последнее уведомление, а список уведомлений очищается
    std::string last_notification() {
        const std::string last = observer.calls.empty() ? "<нет>" : observer.calls.back();
        observer.calls.clear();
        return last;
    }

    int count_calls = 0;
    ss::core::FunctionRegistry functions;
    Spreadsheet sheet{functions};
    RecordingObserver observer;
};

}  // namespace

TEST_F(SpreadsheetTest, StoresNumbersTextAndFormulas) {
    input("A1", "12");
    input("A2", "hello");
    input("A3", "=A1*2");
    EXPECT_EQ(sheet.value(at("A1"))->as_number(), 12);
    EXPECT_EQ(shown("A2"), "hello");
    EXPECT_EQ(sheet.value(at("A3"))->as_number(), 24);
}

TEST_F(SpreadsheetTest, RawKeepsWhatUserTyped) {
    input("A1", "=a1:b1 = 0");
    input("B1", "'12");
    EXPECT_EQ(sheet.raw(at("A1")), "=a1:b1 = 0");
    EXPECT_EQ(sheet.raw(at("B1")), "'12");
    EXPECT_EQ(sheet.raw(at("C1")), "");
}

TEST_F(SpreadsheetTest, FormulaTextIsNormalized) {
    input("A1", "=sum( a1:b2 ) * (2)");
    input("B1", "5");
    EXPECT_EQ(sheet.formula_text(at("A1")), "=SUM(A1:B2)*2");
    EXPECT_EQ(sheet.formula_text(at("B1")), "");
    EXPECT_EQ(sheet.formula_text(at("C1")), "");
}

TEST_F(SpreadsheetTest, EmptyCell) {
    EXPECT_TRUE(sheet.value(at("Z99"))->is_empty());
    EXPECT_EQ(sheet.used_range(), std::nullopt);
}

TEST_F(SpreadsheetTest, ChainIsRecalculated) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("C1", "=B1*10");
    EXPECT_EQ(shown("C1"), "20");
    input("A1", "5");
    EXPECT_EQ(shown("B1"), "6");
    EXPECT_EQ(shown("C1"), "60");
}

TEST_F(SpreadsheetTest, NotifiesExactlyAffectedCells) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("C1", "=B1+1");
    input("D1", "=7");
    observer.calls.clear();

    input("A1", "2");
    ASSERT_EQ(observer.calls.size(), 1U);  // одно уведомление на одну операцию
    EXPECT_EQ(last_notification(), "A1 B1 C1");

    input("D1", "8");
    EXPECT_EQ(last_notification(), "D1");
}

TEST_F(SpreadsheetTest, UnrelatedFormulaIsNotRecalculated) {
    input("A1", "1");
    input("C1", "1");
    input("D1", "=COUNTCALLS(C1)");
    EXPECT_EQ(count_calls, 1);

    input("A1", "2");
    input("B5", "text");
    EXPECT_EQ(count_calls, 1);

    input("C1", "2");
    EXPECT_EQ(count_calls, 2);
    EXPECT_EQ(shown("D1"), "2");
}

TEST_F(SpreadsheetTest, DiamondIsRecalculatedOnce) {
    input("A1", "1");
    input("B1", "=A1+1");
    input("C1", "=A1*2");
    input("D1", "=COUNTCALLS(B1+C1)");
    count_calls = 0;
    input("A1", "10");
    EXPECT_EQ(count_calls, 1);
    EXPECT_EQ(shown("D1"), "31");
}

TEST_F(SpreadsheetTest, InputIntoEmptyCellOfRangeUpdatesSum) {
    input("B1", "=SUM(A1:A5)");
    EXPECT_EQ(shown("B1"), "0");
    input("A3", "4");
    EXPECT_EQ(shown("B1"), "4");
    EXPECT_EQ(last_notification(), "A3 B1");
}

TEST_F(SpreadsheetTest, CycleAndItsDependents) {
    input("A1", "=B1");
    input("B1", "=A1");
    EXPECT_EQ(shown("A1"), "#CYCLE!");
    EXPECT_EQ(shown("B1"), "#CYCLE!");

    input("C1", "=A1+1");
    EXPECT_EQ(shown("C1"), "#CYCLE!");

    input("B1", "3");
    EXPECT_EQ(shown("A1"), "3");
    EXPECT_EQ(shown("B1"), "3");
    EXPECT_EQ(shown("C1"), "4");
}

TEST_F(SpreadsheetTest, BrokenFormulaChangesNothing) {
    input("A1", "=B1*2");
    input("B1", "3");
    observer.calls.clear();

    EXPECT_THROW(input("A1", "=1+"), ss::core::ParseError);

    EXPECT_EQ(sheet.raw(at("A1")), "=B1*2");
    EXPECT_EQ(shown("A1"), "6");
    EXPECT_TRUE(observer.calls.empty());
    // граф тоже прежний: A1 всё ещё следит за B1
    input("B1", "4");
    EXPECT_EQ(shown("A1"), "8");
}

TEST_F(SpreadsheetTest, BrokenInputIntoEmptyCellLeavesItEmpty) {
    EXPECT_THROW(input("A1", "=(1"), ss::core::ParseError);
    EXPECT_EQ(sheet.raw(at("A1")), "");
    EXPECT_EQ(sheet.used_range(), std::nullopt);
    EXPECT_TRUE(observer.calls.empty());
}

TEST_F(SpreadsheetTest, HugeRangeChangesNothing) {
    input("A1", "=B1");
    observer.calls.clear();
    EXPECT_THROW(input("A1", "=SUM(A1:ZZ65536)"), ss::core::LimitError);
    EXPECT_EQ(sheet.raw(at("A1")), "=B1");
    EXPECT_TRUE(observer.calls.empty());
}

TEST_F(SpreadsheetTest, AddressOutsideSheetIsRejected) {
    EXPECT_THROW(sheet.set_input(CellAddress{.row = 0, .col = ss::core::max_cols}, "1"),
                 ss::core::LimitError);
    EXPECT_EQ(sheet.used_range(), std::nullopt);
}

TEST_F(SpreadsheetTest, ClearingCellUpdatesDependents) {
    input("A1", "5");
    input("B1", "=A1*2");
    input("C1", R"(=A1="")");
    sheet.clear(at("A1"));
    EXPECT_TRUE(sheet.value(at("A1"))->is_empty());
    EXPECT_EQ(shown("B1"), "0");
    EXPECT_EQ(shown("C1"), "1");
    EXPECT_EQ(last_notification(), "A1 B1 C1");
}

TEST_F(SpreadsheetTest, EmptyInputClearsCell) {
    input("A1", "5");
    input("A1", "  ");
    EXPECT_EQ(sheet.raw(at("A1")), "");
    EXPECT_EQ(sheet.used_range(), std::nullopt);
}

TEST_F(SpreadsheetTest, ClearingEmptyCellIsSilent) {
    sheet.clear(at("A1"));
    EXPECT_TRUE(observer.calls.empty());
}

TEST_F(SpreadsheetTest, FormulaReplacedByNumberStopsFollowing) {
    input("A1", "1");
    input("B1", "=A1");
    input("B1", "100");
    observer.calls.clear();
    input("A1", "2");
    EXPECT_EQ(last_notification(), "A1");
    EXPECT_EQ(shown("B1"), "100");
}

TEST_F(SpreadsheetTest, UsedRangeAndForEachCell) {
    input("C3", "1");
    input("B2", "2");
    input("D2", "=1");
    EXPECT_EQ(sheet.used_range(), ss::core::Range(at("B2"), at("D3")));
    std::vector<CellAddress> cells;
    sheet.for_each_cell([&cells](CellAddress address) { cells.push_back(address); });
    EXPECT_EQ(join(cells), "B2 D2 C3");
}

TEST_F(SpreadsheetTest, RecalculateAllEvaluatesEveryFormulaOnce) {
    input("A1", "2");
    input("B1", "=COUNTCALLS(A1)");
    input("C1", "=COUNTCALLS(5)");
    input("D1", "=B1+1");
    count_calls = 0;
    observer.calls.clear();

    sheet.recalculate_all();
    EXPECT_EQ(count_calls, 2);
    EXPECT_EQ(last_notification(), "B1 C1 D1");
    EXPECT_EQ(shown("D1"), "3");
}

// так пишут в таблицу форматы файлов: только через узкий интерфейс
TEST_F(SpreadsheetTest, WorksThroughCellSinkAndSource) {
    ss::core::CellSink& sink = sheet;
    sink.set_input(at("A1"), "3");
    sink.set_input(at("A2"), "=A1*A1");
    const ss::core::CellSource& source = sheet;
    EXPECT_EQ(source.value(at("A2"))->as_number(), 9);
    EXPECT_EQ(source.raw(at("A2")), "=A1*A1");
}

TEST_F(SpreadsheetTest, ObserverMayUnsubscribeItselfDuringNotification) {
    class OneShot : public RecordingObserver {
    public:
        explicit OneShot(Spreadsheet& sheet) : sheet_(sheet) {}
        void on_cells_changed(std::span<const CellAddress> cells) override {
            RecordingObserver::on_cells_changed(cells);
            sheet_.unsubscribe(this);
        }

    private:
        Spreadsheet& sheet_;
    };
    OneShot one_shot(sheet);
    sheet.subscribe(&one_shot);
    RecordingObserver after;
    sheet.subscribe(&after);

    input("A1", "1");
    input("A2", "2");
    EXPECT_EQ(one_shot.calls.size(), 1U);
    EXPECT_EQ(after.calls.size(), 2U);  // соседа отписка не задела
    EXPECT_EQ(observer.calls.size(), 2U);
    sheet.unsubscribe(&after);
}

TEST_F(SpreadsheetTest, ObserverMayUnsubscribeAnotherOne) {
    RecordingObserver victim;
    class Killer : public ss::core::CellObserver {
    public:
        Killer(Spreadsheet& sheet, RecordingObserver& victim) : sheet_(sheet), victim_(victim) {}
        void on_cells_changed(std::span<const CellAddress> /*cells*/) override {
            sheet_.unsubscribe(&victim_);
        }

    private:
        Spreadsheet& sheet_;
        RecordingObserver& victim_;
    };
    Killer killer(sheet, victim);
    sheet.subscribe(&killer);
    sheet.subscribe(&victim);

    input("A1", "1");
    EXPECT_TRUE(victim.calls.empty());
    sheet.unsubscribe(&killer);
}

TEST_F(SpreadsheetTest, SubscribeTwiceNotifiesOnce) {
    sheet.subscribe(&observer);
    input("A1", "1");
    EXPECT_EQ(observer.calls.size(), 1U);
}
