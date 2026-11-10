// Учебные расширения ядра, по одному на каждую точку расширения: функция,
// тип значения, формат файла, формат чисел. Каждое сделано новым классом
// и одной строкой регистрации, ни один существующий класс для них
// не менялся (OCP).

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
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
#include "core/io/file_writer.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format.hpp"
#include "core/number_format_registry.hpp"
#include "core/spreadsheet.hpp"
#include "core/text.hpp"
#include "core/value.hpp"
#include "core/value_parser.hpp"

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

// 3. Запись в тот же TSV: значения через таб, как при экспорте в CSV
class TsvWriter final : public ss::core::FileWriter {
public:
    std::string_view name() const override {
        return "TSV";
    }

    std::vector<std::string> extensions() const override {
        return {".tsv"};
    }

    void save(std::ostream& out, const ss::core::CellSource& source) const override {
        const std::optional<ss::core::Range> used = source.used_range();
        if (!used) {
            return;
        }
        for (int row = 0; row <= used->to().row; ++row) {
            for (int col = 0; col <= used->to().col; ++col) {
                out << (col > 0 ? "\t" : "") << source.value({.row = row, .col = col})->to_text();
            }
            out << '\n';
        }
    }

    // формул в файле нет, значит это только экспорт
    bool keeps_formulas() const override {
        return false;
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

// 5. ROUND(число [, знаков]): округление до нужного числа знаков
class RoundFunction final : public ss::core::Function {
public:
    std::string_view name() const override {
        return "ROUND";
    }

    ss::core::ValuePtr call(std::span<const ss::core::Expr* const> args,
                            ss::core::EvalContext& context) const override {
        if (args.empty() || args.size() > 2) {
            return ss::core::make_error(ss::core::ErrorCode::Value);
        }
        double numbers[2] = {0, 0};
        for (std::size_t i = 0; i < args.size(); ++i) {
            const ss::core::ValuePtr value = context.evaluate(*args[i]);
            if (value->is_error()) {
                return value;
            }
            const std::optional<double> number = value->as_number();
            if (!number) {
                return ss::core::make_error(ss::core::ErrorCode::Value);
            }
            numbers[i] = *number;
        }
        const double scale = std::pow(10.0, std::trunc(numbers[1]));
        return ss::core::make_number_or_error(std::round(numbers[0] * scale) / scale);
    }
};

// 6. Новый тип значения: логический. В арифметике и сравнениях это 1 или 0,
// поэтому Evaluator и функции про него знать не обязаны.
class BoolValue final : public ss::core::Value {
public:
    explicit BoolValue(bool value) : value_(value) {}

    std::optional<double> as_number() const override {
        return value_ ? 1.0 : 0.0;
    }

    std::string to_text() const override {
        return value_ ? "TRUE" : "FALSE";
    }

    std::string display(const ss::core::NumberFormat& /*format*/) const override {
        return to_text();
    }

    std::optional<std::partial_ordering> compare(const ss::core::Value& other) const override {
        return other.compare_with_number(*as_number());
    }

    std::optional<std::partial_ordering> compare_with_number(double lhs) const override {
        return lhs <=> *as_number();
    }

    std::optional<std::partial_ordering> compare_with_empty() const override {
        return 0.0 <=> *as_number();
    }

private:
    bool value_;
};

// чтобы TRUE и FALSE можно было ввести прямо в ячейку
class BoolParser final : public ss::core::ValueParser {
public:
    ss::core::ValuePtr parse(std::string_view text) const override {
        const std::string upper = ss::core::to_upper_ascii(text);
        if (upper == "TRUE") {
            return std::make_shared<const BoolValue>(true);
        }
        if (upper == "FALSE") {
            return std::make_shared<const BoolValue>(false);
        }
        return nullptr;
    }
};

class ExtensionsTest : public ::testing::Test {
protected:
    ExtensionsTest() {
        ss::core::register_builtins(functions);
        ss::core::register_value_parsers(value_parsers);
        value_parsers.add(std::make_shared<const BoolParser>());
        // та самая одна строка регистрации
        functions.add("COUNT", [] { return std::make_shared<CountFunction>(); });
        functions.add("ROUND", [] { return std::make_shared<RoundFunction>(); });

        ss::core::register_formats(formats);
        formats.add_reader(std::make_shared<const TsvReader>());
        formats.add_writer(std::make_shared<const TsvWriter>());

        ss::core::register_number_formats(number_formats);
        number_formats.add(
            "sci", [](int digits) { return std::make_shared<const ScientificFormat>(digits); });
    }

    std::string shown(std::string_view address) const {
        return sheet.value(at(address))->to_text();
    }

    ss::core::FunctionRegistry functions;
    ss::core::ValueParserRegistry value_parsers;
    ss::core::FormatRegistry formats;
    ss::core::NumberFormatRegistry number_formats;
    Spreadsheet sheet{functions, value_parsers};
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

TEST_F(ExtensionsTest, RoundFunction) {
    sheet.set_input(at("A1"), "3.14159");
    sheet.set_input(at("B1"), "=ROUND(A1, 2)");
    sheet.set_input(at("B2"), "=round(A1)");
    sheet.set_input(at("B3"), "=ROUND(1234.5, -2)");
    sheet.set_input(at("B4"), "=ROUND(2.5)+ROUND(-2.5)");
    EXPECT_EQ(shown("B1"), "3.14");
    EXPECT_EQ(shown("B2"), "3");
    EXPECT_EQ(shown("B3"), "1200");
    EXPECT_EQ(shown("B4"), "0");
    // пересчитывается вместе со своей ячейкой, как встроенные
    sheet.set_input(at("A1"), "2.71828");
    EXPECT_EQ(shown("B1"), "2.72");
}

TEST_F(ExtensionsTest, RoundRejectsBadArguments) {
    sheet.set_input(at("A1"), "=ROUND()");
    sheet.set_input(at("A2"), "=ROUND(1, 2, 3)");
    sheet.set_input(at("A3"), "=ROUND(\"x\")");
    sheet.set_input(at("A4"), "=ROUND(1/0, 2)");
    EXPECT_EQ(shown("A1"), "#VALUE!");
    EXPECT_EQ(shown("A2"), "#VALUE!");
    EXPECT_EQ(shown("A3"), "#VALUE!");
    EXPECT_EQ(shown("A4"), "#DIV/0!");
}

TEST_F(ExtensionsTest, BoolIsEnteredIntoCell) {
    sheet.set_input(at("A1"), "true");
    sheet.set_input(at("A2"), "FALSE");
    sheet.set_input(at("A3"), "truth");
    EXPECT_EQ(shown("A1"), "TRUE");
    EXPECT_EQ(shown("A2"), "FALSE");
    // остальное по-прежнему текст, число или дата
    EXPECT_EQ(sheet.value(at("A3"))->as_number(), std::nullopt);
    sheet.set_input(at("A4"), "12");
    sheet.set_input(at("A5"), "2024-03-15");
    EXPECT_EQ(sheet.value(at("A4"))->as_number(), 12.0);
    EXPECT_TRUE(sheet.value(at("A5"))->as_date().has_value());
}

// Evaluator и встроенные функции про новый тип не знают, но работают с ним
TEST_F(ExtensionsTest, BoolWorksInFormulas) {
    sheet.set_input(at("A1"), "TRUE");
    sheet.set_input(at("A2"), "FALSE");
    sheet.set_input(at("B1"), "=IF(A1, \"да\", \"нет\")");
    sheet.set_input(at("B2"), "=IF(A2, \"да\", \"нет\")");
    sheet.set_input(at("B3"), "=A1+A1+A2");
    sheet.set_input(at("B4"), "=SUM(A1:A2)");
    sheet.set_input(at("B5"), "=A1>A2");
    sheet.set_input(at("B6"), "=A1=1");
    sheet.set_input(at("B7"), "=A2=C9");
    EXPECT_EQ(shown("B1"), "да");
    EXPECT_EQ(shown("B2"), "нет");
    EXPECT_EQ(shown("B3"), "2");
    EXPECT_EQ(shown("B4"), "1");
    EXPECT_EQ(shown("B5"), "1");
    EXPECT_EQ(shown("B6"), "1");
    // FALSE равно пустой ячейке, как и ноль
    EXPECT_EQ(shown("B7"), "1");
    // формат чисел на него не действует
    EXPECT_EQ(sheet.value(at("A1"))->display(*number_formats.create("fixed:2")), "TRUE");
}

TEST_F(ExtensionsTest, TsvWriterExportsValues) {
    sheet.set_input(at("A1"), "name");
    sheet.set_input(at("B1"), "=1+1");
    sheet.set_input(at("B2"), "TRUE");
    const ss::core::FileWriter& writer = formats.writer_for("out.tsv");
    EXPECT_EQ(writer.name(), "TSV");
    EXPECT_FALSE(writer.keeps_formulas());
    std::ostringstream out;
    writer.save(out, sheet);
    EXPECT_EQ(out.str(), "name\t2\n\tTRUE\n");

    // и читается обратно своим же читателем
    Spreadsheet loaded{functions, value_parsers};
    std::istringstream in(out.str());
    formats.reader_for("out.tsv").load(in, loaded);
    EXPECT_EQ(loaded.value(at("B1"))->to_text(), "2");
    EXPECT_EQ(loaded.value(at("B2"))->to_text(), "TRUE");
}
