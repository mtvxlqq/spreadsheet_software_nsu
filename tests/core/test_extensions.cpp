// Учебные расширения ядра. Каждое сделано новым классом и одной строкой
// регистрации, ни один существующий класс для них не менялся (OCP).

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/address.hpp"
#include "core/builtins.hpp"
#include "core/cell_io.hpp"
#include "core/errors.hpp"
#include "core/eval_context.hpp"
#include "core/function.hpp"
#include "core/function_registry.hpp"
#include "core/io/file_reader.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format.hpp"
#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/text.hpp"
#include "core/value.hpp"

using ss::core::CellAddress;
using ss::core::Spreadsheet;

namespace {

CellAddress at(std::string_view text) {
    return CellAddress::parse(text);
}

// 1. COUNT: сколько среди аргументов чисел. Пустые, строки и ошибки
// не считаются, как в Excel.
class CountFunction final : public ss::core::Function {
public:
    std::string_view name() const override {
        return "COUNT";
    }

    ss::core::ValuePtr call(std::span<const ss::core::Expr* const> args,
                            ss::core::EvalContext& context) const override {
        if (args.empty()) {
            return ss::core::make_error(ss::core::ErrorCode::Value);
        }
        int count = 0;
        for (const ss::core::Expr* arg : args) {
            for (const ss::core::ValuePtr& value : context.evaluate_flat(*arg).values) {
                if (!value->is_empty() && value->as_number().has_value()) {
                    ++count;
                }
            }
        }
        return ss::core::make_number(count);
    }
};

// 2. TSV: поля через таб, без кавычек. Как и CSV, всё читается
// литералами, формулы не выполняются.
class TsvReader final : public ss::core::FileReader {
public:
    std::string_view name() const override {
        return "TSV";
    }

    std::vector<std::string> extensions() const override {
        return {".tsv"};
    }

    void load(std::istream& in, ss::core::CellSink& sink) const override {
        std::string line;
        int row = 0;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            int col = 0;
            std::size_t start = 0;
            while (start <= line.size()) {
                const std::size_t tab = std::min(line.find('\t', start), line.size());
                put(sink, {.row = row, .col = col}, line.substr(start, tab - start),
                    static_cast<std::size_t>(row) + 1);
                start = tab + 1;
                ++col;
            }
            ++row;
        }
    }

private:
    static void put(ss::core::CellSink& sink, CellAddress address, const std::string& field,
                    std::size_t line) {
        const std::string_view text = ss::core::trim_spaces(field);
        if (text.empty()) {
            return;
        }
        if (!address.is_valid()) {
            throw ss::core::FileFormatError("таблица не помещается в лист", line);
        }
        // апостроф впереди, чтобы "=1+1" осталось текстом
        const bool needs_quote = text.front() == '=' || text.front() == '\'';
        sink.set_input(address, needs_quote ? "'" + std::string(text) : field);
    }
};

// 4. Научная запись: 12345 при двух знаках это "1.23e+04"
class ScientificFormat final : public ss::core::NumberFormat {
public:
    explicit ScientificFormat(int digits) : digits_(digits) {}

    std::string format(double value) const override {
        return std::format("{:.{}e}", value, digits_);
    }

private:
    int digits_;
};

class ExtensionsTest : public ::testing::Test {
protected:
    ExtensionsTest() {
        ss::core::register_builtins(functions);
        // та самая одна строка регистрации
        functions.add("COUNT", [] { return std::make_shared<CountFunction>(); });

        ss::core::register_formats(formats);
        formats.add_reader(std::make_shared<const TsvReader>());

        ss::core::register_number_formats(number_formats);
        number_formats.add(
            "sci", [](int digits) { return std::make_shared<const ScientificFormat>(digits); });
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    ss::core::FormatRegistry formats;
    ss::core::NumberFormatRegistry number_formats;
    Spreadsheet sheet{functions};
};

}  // namespace

TEST_F(ExtensionsTest, CountCountsOnlyNumbers) {
    sheet.set_input(at("A1"), "1");
    sheet.set_input(at("A2"), "text");
    sheet.set_input(at("A3"), "=1/0");
    sheet.set_input(at("A5"), "=2+2");
    sheet.set_input(at("B1"), "=COUNT(A1:A5)");
    EXPECT_EQ(shown("B1"), "2");
    sheet.set_input(at("B2"), "=count(A1:A5, 7, A1)");
    EXPECT_EQ(shown("B2"), "4");
    sheet.set_input(at("B3"), "=COUNT()");
    EXPECT_EQ(shown("B3"), "#VALUE!");
}

TEST_F(ExtensionsTest, CountIsRecalculatedLikeBuiltins) {
    sheet.set_input(at("B1"), "=COUNT(A1:A3)");
    EXPECT_EQ(shown("B1"), "0");
    // ввод в пустую ячейку диапазона пересчитывает COUNT
    sheet.set_input(at("A2"), "5");
    EXPECT_EQ(shown("B1"), "1");
}

TEST_F(ExtensionsTest, TsvIsChosenByExtension) {
    EXPECT_EQ(formats.reader_for("data.TSV").name(), "TSV");
    // старые форматы на месте
    EXPECT_EQ(formats.reader_for("data.csv").name(), "CSV");
}

TEST_F(ExtensionsTest, TsvLoadsLiterals) {
    std::istringstream in("name\tvalue\r\nx\t1\ny\t=1+1\n\t\tlast\n");
    formats.reader_for("t.tsv").load(in, sheet);
    EXPECT_EQ(shown("A1"), "name");
    EXPECT_EQ(shown("B2"), "1");
    EXPECT_TRUE(sheet.value(at("B2"))->as_number().has_value());
    EXPECT_EQ(shown("B3"), "=1+1");
    EXPECT_EQ(shown("C4"), "last");
    EXPECT_EQ(sheet.raw(at("A4")), "");
}

TEST_F(ExtensionsTest, ScientificFormatIsCreatedBySpec) {
    const auto scientific = number_formats.create(number_formats.make_spec("sci", 2));
    sheet.set_input(at("A1"), "12345");
    sheet.set_input(at("A2"), "=1/3");
    sheet.set_input(at("A3"), "text");
    sheet.set_input(at("A4"), "=1/0");
    EXPECT_EQ(sheet.value(at("A1"))->display(*scientific), "1.23e+04");
    EXPECT_EQ(sheet.value(at("A2"))->display(*scientific), "3.33e-01");
    // на строки и ошибки формат чисел не действует
    EXPECT_EQ(sheet.value(at("A3"))->display(*scientific), "text");
    EXPECT_EQ(sheet.value(at("A4"))->display(*scientific), "#DIV/0!");
    // старые форматы на месте
    EXPECT_EQ(sheet.value(at("A2"))->display(*number_formats.create("fixed:2")), "0.33");
    EXPECT_EQ(sheet.value(at("A2"))->display(*number_formats.create("")), "0.3333333333");
}
